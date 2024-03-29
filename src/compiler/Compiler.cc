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

#include "compiler/Compiler.h"
#include "runtime/objects/Function.h"
#include "runtime/objects/String.h"
#include "utilities/strings.h"

#include <climits>

SIF_NAMESPACE_BEGIN

Compiler::Compiler(const CompilerConfig &config) : _config(config), _scopeDepth(0) {}

Strong<Bytecode> Compiler::compile(const Statement &statement) {
    _frames.push_back({MakeStrong<Bytecode>(), {}, {}});
    addLocal();

    statement.accept(*this);
    addReturn();

    return _errors.size() > 0 ? nullptr : _frames.back().bytecode;
}

const Set<std::string> &Compiler::globals() const { return _globals; }

const std::vector<Error> &Compiler::errors() const { return _errors; }

Bytecode &Compiler::bytecode() { return *_frames.back().bytecode; }
std::vector<Compiler::Local> &Compiler::locals() { return _frames.back().locals; }
std::vector<Function::Capture> &Compiler::captures() { return _frames.back().captures; }

void Compiler::error(const Node &node, const std::string &message) {
    _errors.push_back(Error(node.location, message));
}

int Compiler::findLocal(const Frame &frame, const std::string &name) {
    for (int i = frame.locals.size() - 1; i >= 0; i--) {
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
    for (int i = 0; i < frame.captures.size(); i++) {
        if (frame.captures[i].index == index && frame.captures[i].isLocal == isLocal) {
            return i;
        }
    }
    frame.captures.push_back({index, isLocal});
    return frame.captures.size() - 1;
}

void Compiler::assign(const Location &location, const std::string &name) {
    uint16_t index;
    Opcode opcode;

    if (_scopeDepth > 0) {
        if (int i = findLocal(_frames.back(), name); i > -1) {
            index = i;
            opcode = Opcode::SetLocal;
        } else if (int i = findCapture(name); i > -1) {
            index = i;
            opcode = Opcode::SetCapture;
        } else {
            addLocal(name);
            index = locals().size() - 1;
            opcode = Opcode::SetLocal;
        }
    } else {
        index = bytecode().addConstant(MakeStrong<String>(name));
        opcode = Opcode::SetGlobal;
    }
    bytecode().add(location, opcode, index);
}

void Compiler::assign(const Variable &variable, const Location &location, const std::string &name) {
    uint16_t index;
    Opcode opcode;

    if (_scopeDepth > 0) {
        switch (variable.scope) {
        case Variable::Scope::Local:
        case Variable::Scope::Unspecified:
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = i;
                opcode = Opcode::SetLocal;
            } else if (int i = findCapture(name); i > -1) {
                index = i;
                opcode = Opcode::SetCapture;
            } else {
                addLocal(name);
                index = locals().size() - 1;
                opcode = Opcode::SetLocal;
            }
            break;
        case Variable::Scope::Global:
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::SetGlobal;
        }
    } else {
        switch (variable.scope) {
        case Variable::Scope::Local:
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = i;
                opcode = Opcode::SetLocal;
            } else {
                addLocal(name);
                index = locals().size() - 1;
                opcode = Opcode::SetLocal;
            }
            break;
        case Variable::Scope::Unspecified:
        case Variable::Scope::Global:
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::SetGlobal;
        }
    }
    bytecode().add(location, opcode, index);
}

void Compiler::resolve(const Call &call, const std::string &name) {
    uint16_t index = 0;
    Opcode opcode;

    if (_scopeDepth > 0) {
        if (int i = findLocal(_frames.back(), name); i > -1) {
            index = i;
            opcode = Opcode::GetLocal;
        } else if (int i = findCapture(name); i > -1) {
            index = i;
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
    bytecode().add(call.location, opcode, index);
}

void Compiler::resolve(const Variable &variable, const std::string &name) {
    uint16_t index = 0;
    Opcode opcode;

    if (_scopeDepth > 0) {
        switch (variable.scope) {
        case Variable::Scope::Unspecified:
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = i;
                opcode = Opcode::GetLocal;
            } else if (int i = findCapture(name); i > -1) {
                index = i;
                opcode = Opcode::GetCapture;
            } else {
                index = bytecode().addConstant(MakeStrong<String>(name));
                opcode = Opcode::GetGlobal;
                _globals.insert(name);
            }
            break;
        case Variable::Scope::Local:
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = i;
                opcode = Opcode::GetLocal;
            } else if (int i = findCapture(name); i > -1) {
                index = i;
                opcode = Opcode::GetCapture;
            } else {
                error(variable,
                      Concat("unused local variable ", Quoted(name), " will always be empty"));
                return;
            }
            break;
        case Variable::Scope::Global:
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::GetGlobal;
            _globals.insert(name);
        }
    } else {
        switch (variable.scope) {
        case Variable::Scope::Local:
            if (int i = findLocal(_frames.back(), name); i > -1) {
                index = i;
                opcode = Opcode::GetLocal;
            } else if (int i = findCapture(name); i > -1) {
                index = i;
                opcode = Opcode::GetCapture;
            } else {
                error(variable,
                      Concat("unused local variable ", Quoted(name), " will always be empty"));
                return;
            }
            break;
        case Variable::Scope::Unspecified:
        case Variable::Scope::Global:
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::GetGlobal;
            _globals.insert(name);
        }
    }
    bytecode().add(variable.location, opcode, index);
}

void Compiler::addReturn() {
    if (bytecode().code().size() == 0 || bytecode().code().back() != Opcode::Return) {
        bytecode().add(Location{0, 0}, Opcode::Empty);
        bytecode().add(Location{0, 0}, Opcode::Return);
    }
}

void Compiler::addLocal(const std::string &name) {
    locals().push_back({name, _scopeDepth});
    bytecode().addLocal(name);
}

void Compiler::beginScope() { _scopeDepth++; }

void Compiler::endScope(const Location &location) {
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

    // Add function name and arguments to locals list.
    addLocal(functionDecl.signature.name());
    for (const auto &term : functionDecl.signature.terms) {
        if (auto arg = std::get_if<Signature::Argument>(&term); arg) {
            if (arg->token.has_value()) {
                addLocal(arg->token.value().text);
            } else {
                addLocal();
            }
        }
    }

    // Compile the body of the function.
    functionDecl.statement->accept(*this);

    // Add implicit return statement if necessary.
    addReturn();

    auto functionCaptures = captures();
    _frames.pop_back();
    endScope(functionDecl.location);

    // Add the function constant to the bytecode, assign it to the given signature name.
    auto function =
        MakeStrong<Function>(functionDecl.signature, functionBytecode, functionCaptures);
    auto constant = bytecode().addConstant(function);
    auto name = functionDecl.signature.name();
    bytecode().add(functionDecl.location, Opcode::Constant, constant);
    assign(functionDecl.location, name);
}

void Compiler::visit(const If &ifStatement) {
    ifStatement.condition->accept(*this);
    auto ifJump = bytecode().add(ifStatement.location, Opcode::JumpIfFalse, 0);
    bytecode().add(ifStatement.location, Opcode::Pop);
    ifStatement.ifStatement->accept(*this);

    auto elseJump = bytecode().add(ifStatement.location, Opcode::Jump, 0);
    bytecode().patchRelativeJump(ifJump);
    bytecode().add(ifStatement.location, Opcode::Pop);
    if (ifStatement.elseStatement) {
        ifStatement.elseStatement->accept(*this);
    }
    bytecode().patchRelativeJump(elseJump);
}

void Compiler::visit(const Try &tryStatement) {
    auto tryJump = bytecode().add(tryStatement.location, Opcode::PushJump, 0);
    tryStatement.statement->accept(*this);
    bytecode().add(tryStatement.location, Opcode::PopJump);
    bytecode().patchAbsoluteJump(tryJump);
}

void Compiler::visit(const Use &useStatement) {
    std::vector<Error> outErrors;
    bool outIsCircular;
    auto source = useStatement.target.encodedString();
    auto module = _config.moduleProvider.module(source, outErrors, outIsCircular);
    if (!module) {
        for (const auto &error : outErrors) {
            _errors.push_back(error);
        }
        return;
    }
    for (const auto &pair : module->values()) {
        const auto &name = pair.first;
        const auto &value = pair.second;
        auto constant = bytecode().addConstant(value);
        bytecode().add(useStatement.location, Opcode::Constant, constant);
        assign(useStatement.location, name);
    }
}

void Compiler::visit(const Using &usingStatement) {
    std::vector<Error> outErrors;
    bool outIsCircular;
    auto source = usingStatement.target.encodedString();
    auto module = _config.moduleProvider.module(source, outErrors, outIsCircular);
    if (!module) {
        for (const auto &error : outErrors) {
            _errors.push_back(error);
        }
        return;
    }

    beginScope();
    for (const auto &pair : module->values()) {
        const auto &name = pair.first;
        const auto &value = pair.second;
        auto constant = bytecode().addConstant(value);
        bytecode().add(usingStatement.location, Opcode::Constant, constant);
        assign(usingStatement.location, name);
    }
    usingStatement.statement->accept(*this);
    endScope(usingStatement.location);
}

void Compiler::visit(const Return &statement) {
    if (statement.expression) {
        statement.expression->accept(*this);
    } else {
        bytecode().add(statement.location, Opcode::Empty);
    }
    bytecode().add(statement.location, Opcode::Return);
}

void Compiler::visit(const Assignment &assignment) {
    if (assignment.subscripts.size() > 0) {
        assignment.variable->accept(*this);
        for (int i = 0; i < assignment.subscripts.size() - 1; i++) {
            assignment.subscripts[i]->accept(*this);
            bytecode().add(assignment.subscripts[i]->location, Opcode::Subscript);
        }
        assignment.subscripts.back()->accept(*this);
        assignment.expression->accept(*this);
        bytecode().add(assignment.location, Opcode::SetSubscript);
    } else {
        assignment.expression->accept(*this);
        auto name = lowercase(assignment.variable->token.text);
        if (name == "it") {
            bytecode().add(assignment.location, Opcode::SetIt);
        } else {
            assign(*assignment.variable, assignment.location, name);
        }
    }
}

void Compiler::visit(const ExpressionStatement &statement) {
    statement.expression->accept(*this);
    bytecode().add(statement.location, Opcode::SetIt);
}

void Compiler::visit(const Repeat &statement) {
    auto nextRepeat = _nextRepeat;
    auto exitRepeat = _exitRepeat;
    _nextRepeat = bytecode().code().size();
    bytecode().add(statement.location, Opcode::Jump, 3);
    _exitRepeat = bytecode().add(statement.location, Opcode::Jump, 0);
    auto repeat = bytecode().code().size();
    statement.statement->accept(*this);
    bytecode().addRepeat(statement.location, repeat);
    bytecode().patchRelativeJump(_exitRepeat);
    _nextRepeat = nextRepeat;
    _exitRepeat = exitRepeat;
}

void Compiler::visit(const RepeatCondition &statement) {
    auto nextRepeat = _nextRepeat;
    auto exitRepeat = _exitRepeat;

    bytecode().add(statement.location, Opcode::Jump, 3);
    _exitRepeat = bytecode().add(statement.location, Opcode::Jump, 0);
    _nextRepeat = bytecode().code().size();

    statement.condition->accept(*this);

    size_t jumpIfCondition = bytecode().code().size();
    if (statement.conditionValue) {
        bytecode().add(statement.location, Opcode::JumpIfFalse, 0);
    } else {
        bytecode().add(statement.location, Opcode::JumpIfTrue, 0);
    }

    bytecode().add(statement.location, Opcode::Pop);
    statement.statement->accept(*this);
    bytecode().addRepeat(statement.location, _nextRepeat);

    bytecode().patchRelativeJump(jumpIfCondition);
    bytecode().add(statement.location, Opcode::Pop);
    bytecode().patchRelativeJump(_exitRepeat);
    _nextRepeat = nextRepeat;
    _exitRepeat = exitRepeat;
}

void Compiler::visit(const RepeatFor &foreach) {
    auto nextRepeat = _nextRepeat;
    auto exitRepeat = _exitRepeat;

    foreach.expression->accept(*this);
    bytecode().add(foreach.location, Opcode::GetEnumerator);

    bytecode().add(foreach.location, Opcode::Jump, 4);
    _exitRepeat = bytecode().add(foreach.location, Opcode::Pop);
    auto jumpExitRepeat = bytecode().add(foreach.location, Opcode::Jump, 0);
    auto jumpIfAtEnd = _nextRepeat = bytecode().add(foreach.location, Opcode::JumpIfAtEnd, 0);
    bytecode().add(foreach.location, Opcode::Enumerate);
    assign(*foreach.variable, foreach.location, foreach.variable->token.text);

    foreach.statement->accept(*this);
    bytecode().addRepeat(foreach.location, _nextRepeat);

    bytecode().patchRelativeJump(jumpIfAtEnd);
    bytecode().patchRelativeJump(jumpExitRepeat);
    bytecode().add(foreach.location, Opcode::Pop);

    _nextRepeat = nextRepeat;
    _exitRepeat = exitRepeat;
}

void Compiler::visit(const ExitRepeat &exit) { bytecode().addRepeat(exit.location, _exitRepeat); }

void Compiler::visit(const NextRepeat &next) { bytecode().addRepeat(next.location, _nextRepeat); }

void Compiler::visit(const Call &call) {
    resolve(call, call.signature.name());
    for (const auto &argument : call.arguments) {
        argument->accept(*this);
    }
    bytecode().add(call.location, Opcode::Call, call.arguments.size());
}

void Compiler::visit(const Grouping &grouping) { grouping.expression->accept(*this); }

void Compiler::visit(const Variable &variable) {
    auto name = lowercase(variable.token.text);
    if (name == "it") {
        bytecode().add(variable.location, Opcode::GetIt);
    } else {
        resolve(variable, name);
    }
}

void Compiler::visit(const Binary &binary) {
    if (binary.binaryOperator == Binary::Operator::And) {
        binary.leftExpression->accept(*this);
        auto jump = bytecode().add(binary.location, Opcode::JumpIfFalse, 0);
        bytecode().add(binary.location, Opcode::Pop);
        binary.rightExpression->accept(*this);
        bytecode().patchRelativeJump(jump);
        return;
    }

    if (binary.binaryOperator == Binary::Operator::Or) {
        binary.leftExpression->accept(*this);
        auto jump = bytecode().add(binary.location, Opcode::JumpIfTrue, 0);
        bytecode().add(binary.location, Opcode::Pop);
        binary.rightExpression->accept(*this);
        bytecode().patchRelativeJump(jump);
        return;
    }

    binary.leftExpression->accept(*this);
    binary.rightExpression->accept(*this);
    switch (binary.binaryOperator) {
    case Binary::Operator::Plus:
        bytecode().add(binary.location, Opcode::Add);
        break;
    case Binary::Operator::Minus:
        bytecode().add(binary.location, Opcode::Subtract);
        break;
    case Binary::Operator::Multiply:
        bytecode().add(binary.location, Opcode::Multiply);
        break;
    case Binary::Operator::Divide:
        bytecode().add(binary.location, Opcode::Divide);
        break;
    case Binary::Operator::Modulo:
        bytecode().add(binary.location, Opcode::Modulo);
        break;
    case Binary::Operator::Exponent:
        bytecode().add(binary.location, Opcode::Exponent);
        break;
    case Binary::Operator::Equal:
        bytecode().add(binary.location, Opcode::Equal);
        break;
    case Binary::Operator::NotEqual:
        bytecode().add(binary.location, Opcode::NotEqual);
        break;
    case Binary::Operator::LessThan:
        bytecode().add(binary.location, Opcode::LessThan);
        break;
    case Binary::Operator::GreaterThan:
        bytecode().add(binary.location, Opcode::GreaterThan);
        break;
    case Binary::Operator::LessThanOrEqual:
        bytecode().add(binary.location, Opcode::LessThanOrEqual);
        break;
    case Binary::Operator::GreaterThanOrEqual:
        bytecode().add(binary.location, Opcode::GreaterThanOrEqual);
        break;
    case Binary::Operator::Subscript:
        bytecode().add(binary.location, Opcode::Subscript);
        break;
    default:
        Abort("unexpected binary operator (", binary.binaryOperator, ")");
    }
}

void Compiler::visit(const Unary &unary) {
    unary.expression->accept(*this);
    switch (unary.unaryOperator) {
    case Unary::Operator::Minus:
        bytecode().add(unary.location, Opcode::Negate);
        break;
    case Unary::Operator::Not:
        bytecode().add(unary.location, Opcode::Not);
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
    bytecode().add(range.location, (range.closed ? Opcode::ClosedRange : Opcode::OpenRange));
}

void Compiler::visit(const ListLiteral &list) {
    for (const auto &expression : list.expressions) {
        expression->accept(*this);
    }
    bytecode().add(list.location, Opcode::List, list.expressions.size());
}

void Compiler::visit(const DictionaryLiteral &dictionary) {
    for (const auto &pair : dictionary.values) {
        pair.first->accept(*this);
        pair.second->accept(*this);
    }
    bytecode().add(dictionary.location, Opcode::Dictionary, dictionary.values.size());
}

static inline Value valueOf(const Token &token) {
    switch (token.type) {
    case Token::Type::StringLiteral: {
        return token.encodedString();
    }
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
        bytecode().add(literal.location, opcode);
        return;
    }

    // Special case generating inline shorts for smaller values.
    if (literal.token.type == Token::Type::IntLiteral) {
        auto value = std::stol(literal.token.text);
        if (value <= USHRT_MAX) {
            bytecode().add(literal.location, Opcode::Short, value);
            return;
        }
    }

    if (literal.token.type == Token::Type::Empty) {
        bytecode().add(literal.location, Opcode::Empty);
        return;
    }

    try {
        auto index = bytecode().addConstant(valueOf(literal.token));
        bytecode().add(literal.location, Opcode::Constant, index);
    } catch (const std::out_of_range &) {
        error(literal, "value is too large or too small");
    }
}

SIF_NAMESPACE_END
