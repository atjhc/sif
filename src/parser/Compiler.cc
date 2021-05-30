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

#include "parser/Compiler.h"
#include "utilities/strings.h"
#include "runtime/objects/String.h"
#include "runtime/objects/Function.h"

CH_NAMESPACE_BEGIN

Compiler::Compiler(Owned<Statement> statement)
    : _depth(0), _bytecode(MakeStrong<Bytecode>()), _statement(std::move(statement)) {}

Strong<Bytecode> Compiler::compile() {
    _statement->accept(*this);
    bytecode().add(Location{1, 1}, Opcode::Empty);
    bytecode().add(Location{1, 1}, Opcode::Return);
    return _errors.size() > 0 ? nullptr : _bytecode;
}

const std::vector<CompileError> &Compiler::errors() const {
    return _errors;
}

Bytecode &Compiler::bytecode() {
    return *_bytecode;
}

void Compiler::error(const Node &node, const std::string &message) {
    _errors.push_back(CompileError(node, message));
}

int Compiler::findLocal(const std::string &name) const {
    for (int i = 0; i < _locals->size(); i++) {
        if (_locals->at(i).name == name) {
            return i;
        }
    }
    return -1;
}

void Compiler::assign(Location location, const std::string &name) {
    uint16_t index;
    Opcode opcode;

    auto it = _globals.find(name);
    if (it == _globals.end()) {
        if (_depth > 0) {
            if (int i = findLocal(name); i > -1) {
                index = i;
            } else {
                _locals->push_back({name, _depth});
                return;
            }
            opcode = Opcode::SetLocal;
        } else {
            index = bytecode().addConstant(MakeStrong<String>(name));
            _globals[name] = index;
            opcode = Opcode::SetGlobal;
        }
    } else {
        index = it->second;
        opcode = Opcode::SetGlobal;
    }
    bytecode().add(location, opcode, index);
}

void Compiler::resolve(Location location, const std::string &name) {
    uint16_t index = 0;
    Opcode opcode;

    auto it = _globals.find(name);
    if (it == _globals.end()) {
        if (_depth > 0) {
            if (int i = findLocal(name); i > -1) {
                index = i;
            } else {
                bytecode().add(location, Opcode::Empty);
                return;
            }
            opcode = Opcode::GetLocal;
        } else {
            index = bytecode().addConstant(MakeStrong<String>(name));
            opcode = Opcode::GetGlobal;
            _globals[name] = index;
        }
    } else {
        index = bytecode().addConstant(MakeStrong<String>(name));
        opcode = Opcode::GetGlobal;
    }
    bytecode().add(location, opcode, index);
}

static inline std::string normalizedName(const Variable &variable) {
    std::ostringstream ss;
    auto it = variable.tokens.begin();
    while (it != variable.tokens.end()) {
        ss << lowercase(it->text);
        if (it != variable.tokens.end() - 1) {
            ss << " ";
        }
        it++;
    }
    return ss.str();
}

void Compiler::visit(const Block &block) {
    for (const auto &statement : block.statements) {
        statement->accept(*this);
    }
}

void Compiler::visit(const FunctionDecl &functionDecl) {
    auto functionBytecode = MakeStrong<Bytecode>();
    auto function = MakeStrong<Function>(functionDecl.signature, functionBytecode);
    auto constant = bytecode().addConstant(function);
    auto name = functionDecl.signature.name();

    bytecode().add(functionDecl.location, Opcode::Constant, constant);
    assign(functionDecl.location, name);

    auto previousBytecode = _bytecode;
    auto previousLocals = std::move(_locals);
    _bytecode = functionBytecode;
    _locals = MakeOwned<std::vector<Local>>();
    _depth++;

    _locals->push_back({"", _depth});
    for (auto term : functionDecl.signature.terms) {
        if (auto arg = std::get_if<FunctionSignature::Argument>(&term)) {
            if (arg->token.has_value()) {
                _locals->push_back({arg->token.value().text, _depth});
            }
        }
    }
    functionDecl.statement->accept(*this);
    
    bytecode().add(functionDecl.location, Opcode::Empty);
    bytecode().add(functionDecl.location, Opcode::Return);

    _depth--;
    _bytecode = previousBytecode;
    _locals = std::move(previousLocals);
}

void Compiler::visit(const If &ifStatement) {
    ifStatement.condition->accept(*this);
    auto ifJump = bytecode().add(ifStatement.location, Opcode::JumpIfFalse, 0);
    bytecode().add(ifStatement.location, Opcode::Pop);
    ifStatement.ifStatement->accept(*this);
    bytecode().add(ifStatement.location, Opcode::Jump, 1);
    bytecode().add(ifStatement.location, Opcode::Pop);

    if (ifStatement.elseStatement) {
        auto elseJump = bytecode().add(ifStatement.location, Opcode::Jump, 0);
        bytecode().patchJump(ifJump);
        bytecode().add(ifStatement.location, Opcode::Pop);
        ifStatement.elseStatement->accept(*this);
        bytecode().patchJump(elseJump);
    } else {
        bytecode().patchJump(ifJump);
    }
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
    assignment.expression->accept(*this);
    assign(assignment.location, normalizedName(*assignment.variable));
}

void Compiler::visit(const ExpressionStatement &statement) {
    statement.expression->accept(*this);
    bytecode().add(statement.location, Opcode::Show);
    bytecode().add(statement.location, Opcode::Pop);
}

void Compiler::visit(const Repeat &statement) {
    _nextRepeat = bytecode().code().size();
    bytecode().add(statement.location, Opcode::Jump, 3);
    _exitRepeat = bytecode().add(statement.location, Opcode::Jump, 0);
    auto repeat = bytecode().code().size();
    statement.statement->accept(*this);
    bytecode().addRepeat(statement.location, repeat);
    bytecode().patchJump(_exitRepeat);
}

void Compiler::visit(const RepeatCondition &statement) {
    bytecode().add(statement.location, Opcode::Jump, 3);
    _exitRepeat = bytecode().add(statement.location, Opcode::Jump, 0);
    _nextRepeat = bytecode().code().size();
    statement.condition->accept(*this);
    size_t jump;
    if (statement.conditionValue) {
        jump = bytecode().add(statement.location, Opcode::JumpIfFalse, 0);
    } else {
        jump = bytecode().add(statement.location, Opcode::JumpIfTrue, 0);
    }
    bytecode().add(statement.location, Opcode::Pop);
    statement.statement->accept(*this);
    bytecode().addRepeat(statement.location, _nextRepeat);
    bytecode().patchJump(jump);
    bytecode().add(statement.location, Opcode::Pop);
    bytecode().patchJump(_exitRepeat);
}

void Compiler::visit(const ExitRepeat &exit) {
    bytecode().addRepeat(exit.location, _exitRepeat);
}

void Compiler::visit(const NextRepeat &next) {
    bytecode().addRepeat(next.location, _nextRepeat);
}

void Compiler::visit(const Call &call) {
    resolve(call.location, call.signature.name());
    for (const auto &argument : call.arguments) {
        argument->accept(*this);
    }
    bytecode().add(call.location, Opcode::Call, call.arguments.size());
}

void Compiler::visit(const Grouping &grouping) {
    grouping.expression->accept(*this);
}

void Compiler::visit(const Variable &variable) {
    resolve(variable.location, normalizedName(variable));
}

void Compiler::visit(const Binary &binary) {
    if (binary.binaryOperator == Binary::Operator::And) {
        binary.leftExpression->accept(*this);
        auto jump = bytecode().add(binary.location, Opcode::JumpIfFalse, 0);
        bytecode().add(binary.location, Opcode::Pop);
        binary.rightExpression->accept(*this);
        bytecode().patchJump(jump);
        return;
    }

    if (binary.binaryOperator == Binary::Operator::Or) {
        binary.leftExpression->accept(*this);
        auto jump = bytecode().add(binary.location, Opcode::JumpIfTrue, 0);
        bytecode().add(binary.location, Opcode::Pop);
        binary.rightExpression->accept(*this);
        bytecode().patchJump(jump);
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

void Compiler::visit(const ListLiteral &list) {
    for (const auto &expression : list.expressions) {
        expression->accept(*this);
    }
    bytecode().add(list.location, Opcode::List, list.expressions.size());
}

static inline Value valueOf(const Token &token) {
    switch (token.type) {
    case Token::Type::StringLiteral: {
        auto string = std::string(token.text.begin() + 1, token.text.end() - 1);
        return MakeStrong<String>(string_from_escaped_string(string));
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
        auto opcode = literal.token.text == "true" ? Opcode::True : Opcode::False;
        bytecode().add(literal.location, opcode);
        return;
    }

    if (literal.token.type == Token::Type::IntLiteral) {
        auto value = std::stol(literal.token.text);
        if (value <= USHRT_MAX) {
            bytecode().add(literal.location, Opcode::Short, value);
            return;
        }
    }

    try {
        auto index = bytecode().addConstant(valueOf(literal.token));
        bytecode().add(literal.location, Opcode::Constant, index);
    } catch (const std::out_of_range &) {
        error(literal, "value is too large or too small");
    }
}

CH_NAMESPACE_END
