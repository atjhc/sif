//
//  Copyright (c) 2021 James Callender
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "sif/compiler/Compiler.h"
#include "sif/runtime/objects/Function.h"
#include "sif/runtime/objects/String.h"
#include "utilities/strings.h"
#include <sif/Utilities.h>

#include <climits>
#include <ranges>

SIF_NAMESPACE_BEGIN

Compiler::Compiler(const CompilerConfig &config) : _config(config), _scopeDepth(0) {}

Strong<Bytecode> Compiler::compile(const Statement &statement) {
    _frames.push_back({MakeStrong<Bytecode>(), {}, {}});
    addLocal();

    statement.accept(*this);
    addImplicitReturnIfNeeded();

    return _failed ? nullptr : _frames.back().bytecode;
}

const Set<std::string> &Compiler::globals() const { return _globals; }

Bytecode &Compiler::bytecode() { return *_frames.back().bytecode; }
std::vector<Compiler::Local> &Compiler::locals() { return _frames.back().locals; }
std::vector<Function::Capture> &Compiler::captures() { return _frames.back().captures; }

void Compiler::error(const SourceRange &range, const std::string &message) {
    _config.errorReporter.report(Error(range, Value(message)));
    _failed = true;
}

int Compiler::findLocal(const Frame &frame, const std::string &name) {
    for (int i = static_cast<int>(frame.locals.size()) - 1; i >= 0; i--) {
        if (frame.locals[i].name == name) {
            return i;
        }
    }
    return -1;
}

int Compiler::findCapture(const std::string &name) {
    if (_frames.size() < 2) {
        return -1;
    }
    int index = -1;
    for (auto it = _frames.end() - 2; it >= _frames.begin(); it--) {
        if (index = findLocal(*it, name); index > -1) {
            it++;
            index = addCapture(*it, index, true);
            it++;
            while (it < _frames.end()) {
                index = addCapture(*it, index, false);
                it++;
            }
            break;
        }
    }
    return index;
}

int Compiler::addCapture(Frame &frame, int index, bool isLocal) {
    for (int i = 0; i < static_cast<int>(frame.captures.size()); i++) {
        if (frame.captures[i].index == index && frame.captures[i].isLocal == isLocal) {
            return i;
        }
    }
    frame.captures.push_back({index, isLocal});
    return static_cast<int>(frame.captures.size()) - 1;
}

void Compiler::assignLocal(const SourceLocation &location, const std::string &name) {
    uint16_t index;
    Opcode opcode;
    if (int i = findLocal(_frames.back(), name); i > -1) {
        index = i;
        opcode = Opcode::SetLocal;
    } else if (int i = findCapture(name); i > -1) {
        index = i;
        opcode = Opcode::SetCapture;
    } else {
        addLocal(name);
        if (locals().size() - 1 > UINT16_MAX) {
            error(location, Format(Errors::TooManyLocalVariables));
            return;
        }
        index = static_cast<uint16_t>(locals().size()) - 1;
        opcode = Opcode::SetLocal;
    }
    bytecode().add(location, opcode, index);
}

void Compiler::assignGlobal(const SourceLocation &location, const std::string &name) {
    auto index = bytecode().addConstant(MakeStrong<String>(name));
    bytecode().add(location, Opcode::SetGlobal, index);
}

void Compiler::assignVariable(const SourceLocation &location, const std::string &name,
                              Optional<Variable::Scope> scope) {
    if (scope == Variable::Scope::Local) {
        assignLocal(location, name);
    } else if (scope == Variable::Scope::Global) {
        assignGlobal(location, name);
    } else if (_scopeDepth > 0) {
        assignLocal(location, name);
    } else if (_config.interactive) {
        assignGlobal(location, name);
    } else {
        assignLocal(location, name);
    }
}

void Compiler::assignFunction(const SourceLocation &location, const std::string &name) {
    if (_scopeDepth > 0) {
        assignLocal(location, name);
    } else {
        assignGlobal(location, name);
    }
}

void Compiler::resolve(const Call &call, const std::string &name) {
    uint16_t index = 0;
    Opcode opcode;

    if (!_config.interactive || _scopeDepth > 0) {
        if (int i = findLocal(_frames.back(), name); i > -1) {
            index = static_cast<uint16_t>(i);
            opcode = Opcode::GetLocal;
        } else if (int i = findCapture(name); i > -1) {
            index = static_cast<uint16_t>(i);
            opcode = Opcode::GetCapture;
        } else {
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::GetGlobal;
        }
    } else {
        index = bytecode().addConstant(MakeStrong<String>(name));
        opcode = Opcode::GetGlobal;
        _globals.insert(name);
    }
    bytecode().add(call.range.start, opcode, index);
}

void Compiler::resolve(const Variable &variable, const std::string &name) {
    uint16_t index = 0;
    Opcode opcode = Opcode::GetGlobal; // initialized to suppress GCC warning

    if (!_config.interactive || _scopeDepth > 0) {
        if (variable.scope) {
            switch (variable.scope.value()) {
            case Variable::Scope::Local:
                if (int i = findLocal(_frames.back(), name); i > -1) {
                    index = static_cast<uint16_t>(i);
                    opcode = Opcode::GetLocal;
                } else if (int i = findCapture(name); i > -1) {
                    index = static_cast<uint16_t>(i);
                    opcode = Opcode::GetCapture;
                } else {
                    error(variable, Format(Errors::UnusedLocalVariable, name));
                    return;
                }
                break;
            case Variable::Scope::Global:
                index = bytecode().addConstant(MakeStrong<String>(name));
                opcode = Opcode::GetGlobal;
                _globals.insert(name);
            }
        } else {
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = static_cast<uint16_t>(i);
                opcode = Opcode::GetLocal;
            } else if (int i = findCapture(name); i > -1) {
                index = static_cast<uint16_t>(i);
                opcode = Opcode::GetCapture;
            } else {
                index = bytecode().addConstant(MakeStrong<String>(name));
                opcode = Opcode::GetGlobal;
                _globals.insert(name);
            }
        }
    } else {
        if (variable.scope && variable.scope.value() == Variable::Scope::Local) {
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = static_cast<uint16_t>(i);
                opcode = Opcode::GetLocal;
            } else if (int i = findCapture(name); i > -1) {
                index = static_cast<uint16_t>(i);
                opcode = Opcode::GetCapture;
            } else {
                error(variable, Format(Errors::UnusedLocalVariable, name));
                return;
            }
        } else {
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::GetGlobal;
            _globals.insert(name);
        }
    }
    bytecode().add(variable.range.start, opcode, index);
}

void Compiler::addImplicitReturnIfNeeded() {
    if (bytecode().code().size() == 0 || bytecode().code().back() != Opcode::Return) {
        bytecode().add(SourceLocation{0, 0}, Opcode::GetIt);
        bytecode().add(SourceLocation{0, 0}, Opcode::Return);
    }
}

void Compiler::addLocal(const std::string &name) {
    locals().push_back({name, _scopeDepth});
    bytecode().addLocal(name);
}

void Compiler::beginScope() { _scopeDepth++; }

void Compiler::endScope(const SourceLocation &location) {
    _scopeDepth--;
    while (locals().size() > 0 && locals().back().scopeDepth > _scopeDepth) {
        locals().pop_back();
        bytecode().add(location, Opcode::Pop);
    }
}

void Compiler::visit(const Block &block) {
    for (const auto &statement : block.statements) {
        statement->accept(*this);
    }
}

void Compiler::visit(const FunctionDecl &functionDecl) {
    // Create a scope, and push a new frame.
    beginScope();
    auto functionBytecode = MakeStrong<Bytecode>();
    _frames.push_back({functionBytecode, {}, {}});

    // Add function name to locals list.
    addLocal(functionDecl.signature.value().name());

    // Visitor to add locals from AssignmentTargets
    struct LocalAdder : AssignmentTarget::Visitor {
        Compiler &compiler;
        LocalAdder(Compiler &c) : compiler(c) {}

        void visit(const VariableTarget &target) override {
            if (target.variable->name) {
                auto name = NormalizeIdentifier(target.variable->name->text);
                if (name == "_") {
                    compiler.addLocal();
                } else {
                    compiler.addLocal(name);
                }
            }
        }

        void visit(const StructuredTarget &target) override {
            for (const auto &nestedTarget : target.targets) {
                nestedTarget->accept(*this);
            }
        }
    };

    // Add all parameter variables to locals
    LocalAdder adder(*this);
    for (const auto &target : functionDecl.targets) {
        target->accept(adder);
    }

    // Compile the body of the function.
    functionDecl.statement->accept(*this);

    // Add implicit return statement if necessary.
    addImplicitReturnIfNeeded();

    auto functionCaptures = captures();
    _frames.pop_back();
    endScope(functionDecl.range.start);

    // Add the function constant to the bytecode, assign it to the given signature name.
    auto function =
        MakeStrong<Function>(functionDecl.signature.value(), functionBytecode, functionCaptures);
    auto constant = bytecode().addConstant(function);
    auto name = functionDecl.signature.value().name();
    bytecode().add(functionDecl.range.start, Opcode::Constant, constant);
    assignFunction(functionDecl.range.start, name);
}

void Compiler::visit(const If &ifStatement) {
    ifStatement.condition->accept(*this);
    auto ifJump = bytecode().add(ifStatement.range.start, Opcode::JumpIfFalse, 0);
    bytecode().add(ifStatement.range.start, Opcode::Pop);
    ifStatement.ifStatement->accept(*this);

    auto elseJump = bytecode().add(ifStatement.range.start, Opcode::Jump, 0);
    bytecode().patchRelativeJump(ifJump);
    bytecode().add(ifStatement.range.start, Opcode::Pop);
    if (ifStatement.elseStatement) {
        ifStatement.elseStatement->accept(*this);
    }
    bytecode().patchRelativeJump(elseJump);
}

void Compiler::visit(const Try &tryStatement) {
    auto tryJump = bytecode().add(tryStatement.range.start, Opcode::PushJump, 0);
    tryStatement.statement->accept(*this);
    bytecode().add(tryStatement.range.start, Opcode::PopJump);
    bytecode().patchAbsoluteJump(tryJump);
}

void Compiler::visit(const Use &useStatement) {
    auto source = useStatement.target.encodedStringLiteralOrWord();
    auto module = _config.moduleProvider.module(source);
    if (!module) {
        return;
    }
    for (const auto &pair : module.value()->values()) {
        const auto &name = pair.first;
        const auto &value = pair.second;
        auto constant = bytecode().addConstant(value);
        bytecode().add(useStatement.range.start, Opcode::Constant, constant);
        assignVariable(useStatement.range.start, name, Variable::Scope::Local);
    }
}

void Compiler::visit(const Using &usingStatement) {
    auto source = usingStatement.target.encodedStringLiteralOrWord();
    auto module = _config.moduleProvider.module(source);
    if (!module) {
        return;
    }
    beginScope();
    for (const auto &pair : module.value()->values()) {
        const auto &name = pair.first;
        const auto &value = pair.second;
        auto constant = bytecode().addConstant(value);
        bytecode().add(usingStatement.range.start, Opcode::Constant, constant);
        assignVariable(usingStatement.range.start, name, Variable::Scope::Local);
    }
    usingStatement.statement->accept(*this);
    endScope(usingStatement.range.start);
}

void Compiler::visit(const Return &statement) {
    if (statement.expression) {
        statement.expression->accept(*this);
    } else {
        bytecode().add(statement.range.start, Opcode::Empty);
    }
    bytecode().add(statement.range.start, Opcode::Return);
}

void Compiler::visit(const Assignment &assignment) {
    assignment.expression->accept(*this);
    if (assignment.targets.size() > 1) {
        uint16_t count;
        if (assignment.targets.size() > UINT16_MAX) {
            error(*assignment.expression, Format(Errors::TooManyAssignmentTargets));
            return;
        }
        count = assignment.targets.size();
        bytecode().add(assignment.expression->range.start, Opcode::UnpackList, count);
    }

    for (auto &&target : std::views::reverse(assignment.targets)) {
        target->accept(*this);
    }
}

void Compiler::visit(const VariableTarget &target) {
    if (!target.variable->name) {
        return;
    }

    if (target.subscripts.size() > 0) {
        target.variable->accept(*this);
        for (int i = 0; i < target.subscripts.size() - 1; i++) {
            target.subscripts[i]->accept(*this);
            bytecode().add(target.subscripts[i]->range.start, Opcode::Subscript);
        }
        target.subscripts.back()->accept(*this);
        bytecode().add(target.variable->range.start, Opcode::SetSubscript);
    } else {
        auto name = NormalizeIdentifier(target.variable->name->text);
        if (name == "it") {
            bytecode().add(target.variable->range.start, Opcode::SetIt);
        } else {
            assignVariable(target.variable->range.start, name, target.variable->scope);
        }
    }
}

void Compiler::visit(const StructuredTarget &target) {
    uint16_t count = target.targets.size();
    bytecode().add(target.range.start, Opcode::UnpackList, count);
    for (auto &&nestedTarget : std::views::reverse(target.targets)) {
        nestedTarget->accept(*this);
    }
}

void Compiler::visit(const ExpressionStatement &statement) {
    statement.expression->accept(*this);
    bytecode().add(statement.range.start, Opcode::SetIt);
}

void Compiler::visit(const Repeat &statement) {
    auto nextRepeat = _nextRepeat;
    _exitPatches.push({});
    _nextRepeat = bytecode().code().size();
    statement.statement->accept(*this);
    bytecode().addRepeat(statement.range.start, _nextRepeat);
    for (auto location : _exitPatches.top()) {
        bytecode().patchRelativeJump(location);
    }
    _nextRepeat = nextRepeat;
    _exitPatches.pop();
}

void Compiler::visit(const RepeatCondition &statement) {
    auto nextRepeat = _nextRepeat;
    _exitPatches.push({});

    _nextRepeat = bytecode().code().size();
    statement.condition->accept(*this);

    size_t jumpIfCondition = bytecode().code().size();
    if (statement.conjunction == RepeatCondition::Conjunction::While) {
        bytecode().add(statement.range.start, Opcode::JumpIfFalse, 0);
    } else {
        bytecode().add(statement.range.start, Opcode::JumpIfTrue, 0);
    }

    bytecode().add(statement.range.start, Opcode::Pop);
    statement.statement->accept(*this);
    bytecode().addRepeat(statement.range.start, _nextRepeat);

    bytecode().patchRelativeJump(jumpIfCondition);
    bytecode().add(statement.range.start, Opcode::Pop);
    for (auto location : _exitPatches.top()) {
        bytecode().patchRelativeJump(location);
    }

    _nextRepeat = nextRepeat;
    _exitPatches.pop();
}

void Compiler::visit(const RepeatFor &foreach) {
    auto nextRepeat = _nextRepeat;
    _exitPatches.push({});

    foreach.expression->accept(*this);
    bytecode().add(foreach.expression->range.start, Opcode::GetEnumerator);
    _nextRepeat = bytecode().add(foreach.expression->range.start, Opcode::JumpIfAtEnd, 0);
    bytecode().add(foreach.expression->range.start, Opcode::Enumerate);
    if (foreach.variables.size() > 1) {
        bytecode().add(foreach.expression->range.start, Opcode::UnpackList,
                       foreach.variables.size());
    }
    for (auto &&variable : std::views::reverse(foreach.variables)) {
        if (variable->name) {
            assignVariable(foreach.expression->range.start, NormalizeIdentifier(variable->name->text),
                           variable->scope);
        }
    }

    foreach.statement->accept(*this);

    bytecode().addRepeat(foreach.range.start, _nextRepeat);
    auto popLocation = bytecode().code().size();
    bytecode().add(foreach.range.start, Opcode::Pop);
    bytecode().patchRelativeJumpTo(_nextRepeat, popLocation);
    for (auto location : _exitPatches.top()) {
        bytecode().patchRelativeJumpTo(location, popLocation);
    }

    _nextRepeat = nextRepeat;
    _exitPatches.pop();
}

void Compiler::visit(const ExitRepeat &exit) {
    _exitPatches.top().push_back(bytecode().add(exit.range.start, Opcode::Jump, 0));
}

void Compiler::visit(const NextRepeat &next) {
    bytecode().addRepeat(next.range.start, _nextRepeat);
}

void Compiler::visit(const Call &call) {
    resolve(call, call.signature.name());
    uint16_t totalCount = 0;
    std::vector<SourceRange> argumentRanges;

    if (_config.enableDebugInfo) {
        argumentRanges.push_back(call.range);
    }

    for (uint16_t i = 0; i < call.arguments.size(); i++) {
        const auto &argument = call.arguments[i];
        if (_config.enableDebugInfo) {
            argumentRanges.push_back(argument->range);
        }
        argument->accept(*this);
        uint16_t count = call.signature.arguments()[i].targets.size();
        if (count > 1) {
            bytecode().add(argument->range.start, Opcode::UnpackList, count);
        }
        totalCount += count;
    }

    auto callLocation = bytecode().add(call.range.start, Opcode::Call, totalCount);
    if (_config.enableDebugInfo && !argumentRanges.empty()) {
        bytecode().addArgumentRanges(callLocation, argumentRanges);
    }
}

void Compiler::visit(const Grouping &grouping) { grouping.expression->accept(*this); }

void Compiler::visit(const Variable &variable) {
    if (!variable.name) {
        return;
    }

    auto name = NormalizeIdentifier(variable.name->text);
    if (name == "it") {
        bytecode().add(variable.range.start, Opcode::GetIt);
    } else {
        resolve(variable, name);
    }
}

void Compiler::visit(const Binary &binary) {
    if (binary.binaryOperator == Binary::Operator::And) {
        binary.leftExpression->accept(*this);
        auto jump = bytecode().add(binary.range.start, Opcode::JumpIfFalse, 0);
        bytecode().add(binary.range.start, Opcode::Pop);
        binary.rightExpression->accept(*this);
        bytecode().patchRelativeJump(jump);
        return;
    }

    if (binary.binaryOperator == Binary::Operator::Or) {
        binary.leftExpression->accept(*this);
        auto jump = bytecode().add(binary.range.start, Opcode::JumpIfTrue, 0);
        bytecode().add(binary.range.start, Opcode::Pop);
        binary.rightExpression->accept(*this);
        bytecode().patchRelativeJump(jump);
        return;
    }

    binary.leftExpression->accept(*this);
    binary.rightExpression->accept(*this);
    switch (binary.binaryOperator) {
    case Binary::Operator::Plus:
        bytecode().add(binary.range.start, Opcode::Add);
        break;
    case Binary::Operator::Minus:
        bytecode().add(binary.range.start, Opcode::Subtract);
        break;
    case Binary::Operator::Multiply:
        bytecode().add(binary.range.start, Opcode::Multiply);
        break;
    case Binary::Operator::Divide:
        bytecode().add(binary.range.start, Opcode::Divide);
        break;
    case Binary::Operator::Modulo:
        bytecode().add(binary.range.start, Opcode::Modulo);
        break;
    case Binary::Operator::Exponent:
        bytecode().add(binary.range.start, Opcode::Exponent);
        break;
    case Binary::Operator::Equal:
        bytecode().add(binary.range.start, Opcode::Equal);
        break;
    case Binary::Operator::NotEqual:
        bytecode().add(binary.range.start, Opcode::NotEqual);
        break;
    case Binary::Operator::LessThan:
        bytecode().add(binary.range.start, Opcode::LessThan);
        break;
    case Binary::Operator::GreaterThan:
        bytecode().add(binary.range.start, Opcode::GreaterThan);
        break;
    case Binary::Operator::LessThanOrEqual:
        bytecode().add(binary.range.start, Opcode::LessThanOrEqual);
        break;
    case Binary::Operator::GreaterThanOrEqual:
        bytecode().add(binary.range.start, Opcode::GreaterThanOrEqual);
        break;
    case Binary::Operator::Subscript:
        bytecode().add(binary.range.start, Opcode::Subscript);
        break;
    default:
        Abort("unexpected binary operator (", binary.binaryOperator, ")");
    }
}

void Compiler::visit(const Unary &unary) {
    unary.expression->accept(*this);
    switch (unary.unaryOperator) {
    case Unary::Operator::Minus:
        bytecode().add(unary.range.start, Opcode::Negate);
        break;
    case Unary::Operator::Not:
        bytecode().add(unary.range.start, Opcode::Not);
        break;
    default:
        break;
    }
}

void Compiler::visit(const RangeLiteral &range) {
    if (range.start) {
        range.start->accept(*this);
    }
    if (range.end) {
        range.end->accept(*this);
    }
    bytecode().add(range.range.start, (range.closed ? Opcode::ClosedRange : Opcode::OpenRange));
}

void Compiler::visit(const ListLiteral &list) {
    for (const auto &expression : list.expressions) {
        expression->accept(*this);
    }
    bytecode().add(list.range.start, Opcode::List, list.expressions.size());
}

void Compiler::visit(const DictionaryLiteral &dictionary) {
    for (const auto &pair : dictionary.values) {
        pair.first->accept(*this);
        pair.second->accept(*this);
    }
    bytecode().add(dictionary.range.start, Opcode::Dictionary, dictionary.values.size());
}

static inline Value valueOf(const Token &token) {
    switch (token.type) {
    case Token::Type::StringLiteral:
    case Token::Type::ClosedInterpolation:
        return token.encodedString();
    case Token::Type::IntLiteral:
        return std::stol(token.text);
    case Token::Type::FloatLiteral:
        return std::stod(token.text);
    default:
        Abort("unexpected token literal ", RawValue(token.type));
    }
}

void Compiler::visit(const Literal &literal) {
    if (literal.token.type == Token::Type::BoolLiteral) {
        auto opcode =
            (lowercase(literal.token.text) == "true" || lowercase(literal.token.text) == "yes")
                ? Opcode::True
                : Opcode::False;
        bytecode().add(literal.range.start, opcode);
        return;
    }

    // Special case generating inline shorts for smaller values.
    if (literal.token.type == Token::Type::IntLiteral) {
        auto value = std::stol(literal.token.text);
        if (value <= USHRT_MAX) {
            bytecode().add(literal.range.start, Opcode::Short, value);
            return;
        }
    }

    if (literal.token.type == Token::Type::Empty) {
        bytecode().add(literal.range.start, Opcode::Empty);
        return;
    }

    try {
        auto index = bytecode().addConstant(valueOf(literal.token));
        bytecode().add(literal.range.start, Opcode::Constant, index);
    } catch (const std::out_of_range &) {
        error(literal, Format(Errors::ValueOutOfRange));
    }
}

void Compiler::visit(const StringInterpolation &interpolation) {
    auto leftIndex = bytecode().addConstant(interpolation.left.encodedString());
    bytecode().add(interpolation.range.start, Opcode::Constant, leftIndex);

    interpolation.expression->accept(*this);

    bytecode().add(interpolation.expression->range.start, Opcode::ToString);
    bytecode().add(interpolation.expression->range.start, Opcode::Add);

    interpolation.right->accept(*this);
    bytecode().add(interpolation.right->range.start, Opcode::Add);
}

SIF_NAMESPACE_END
