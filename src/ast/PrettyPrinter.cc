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

#include "ast/PrettyPrinter.h"

CH_NAMESPACE_BEGIN

using namespace sif;

PrettyPrinter::PrettyPrinter(const PrettyPrinterConfig &c) : _config(c), out(c.out) {}

void PrettyPrinter::print(const Expression &expression) { expression.accept(*this); }

void PrettyPrinter::print(const Statement &statement) { statement.accept(*this); }

void PrettyPrinter::_printBlock(const Statement &statement) {
    _indentLevel++;
    out << std::endl << indentString();
    statement.accept(*this);
    out << std::endl;
    _indentLevel--;
}

void PrettyPrinter::visit(const Block &block) {
    auto it = block.statements.begin();
    while (it != block.statements.end()) {
        if (it != block.statements.begin()) {
            out << indentString();
        }
        (*it)->accept(*this);
        it++;
        if (it != block.statements.end()) {
            out << std::endl;
        }
    }
}

void PrettyPrinter::visit(const FunctionDecl &functionDecl) {
    out << "function " << functionDecl.signature.description();
    _printBlock(*functionDecl.statement);
    out << indentString() << "end function";
}

void PrettyPrinter::visit(const If &ifs) {
    out << "if ";
    ifs.condition->accept(*this);
    out << " then";

    _printBlock(*ifs.ifStatement);

    if (ifs.elseStatement) {
        out << indentString() << "else";
        _printBlock(*ifs.elseStatement);
    }
    out << indentString() << "end if";
}

void PrettyPrinter::visit(const Return &statement) {
    out << "return";
    if (statement.expression) {
        out << " ";
        statement.expression->accept(*this);
    }
}

void PrettyPrinter::visit(const Assignment &set) {
    out << "set ";
    set.variable->accept(*this);
    out << " to ";
    set.expression->accept(*this);
}

void PrettyPrinter::visit(const ExpressionStatement &statement) {
    statement.expression->accept(*this);
}

void PrettyPrinter::visit(const Repeat &repeat) {
    out << "repeat forever";
    _printBlock(*repeat.statement);
    out << indentString() << "end repeat";
}

// void PrettyPrinter::visit(const RepeatCount &r) {
//     out << "repeat ";
//     r.countExpression->accept(*this);
//     out << std::endl;
//     print(*r.statements);
//     out << indentString() << "end repeat";
// }

void PrettyPrinter::visit(const RepeatCondition &repeat) {
    out << "repeat";
    if (repeat.conditionValue) {
        out << " while ";
    } else {
        out << " until ";
    }
    repeat.condition->accept(*this);
    _printBlock(*repeat.statement);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const ExitRepeat &) { out << "exit repeat"; }

void PrettyPrinter::visit(const NextRepeat &) { out << "next repeat"; }

void PrettyPrinter::visit(const Call &call) {
    auto tokensIt = call.tokens.begin();
    auto argsIt = call.arguments.begin();

    auto it = call.signature.terms.begin();
    while (it < call.signature.terms.end()) {
        const auto &term = *it;
        bool skip = false;
        std::visit(
            Overload {
                [&](Token token) { out << token.text; },
                [&](FunctionSignature::Argument argument) {
                    (*argsIt)->accept(*this);
                    argsIt++;
                },
                [&](FunctionSignature::Choice choice) {
                    out << tokensIt->value().text;
                    tokensIt++;
                },
                [&](FunctionSignature::Option argument) {
                    if (tokensIt->has_value()) {
                        out << tokensIt->value().text;
                    } else {
                        skip = true;
                    }
                    tokensIt++;
                },
            },
            term
        );
        it++;
        if (it != call.signature.terms.end() && !skip) {
            out << " ";
        }
    }
}

void PrettyPrinter::visit(const Grouping &grouping) {
    out << "(";
    grouping.expression->accept(*this);
    out << ")";
}

void PrettyPrinter::visit(const Variable &variable) {
    out << variable.token.text;
    if (variable.typeName.has_value()) {
        out << ": " << variable.typeName.value().text;
    }
}

void PrettyPrinter::visit(const Binary &binary) {
    binary.leftExpression->accept(*this);
    switch (binary.binaryOperator) {
    case Binary::And:
        out << " and ";
        break;
    case Binary::Or:
        out << " or ";
        break;
    case Binary::Equal:
        out << " = ";
        break;
    case Binary::NotEqual:
        out << " != ";
        break;
    case Binary::LessThan:
        out << " < ";
        break;
    case Binary::GreaterThan:
        out << " > ";
        break;
    case Binary::LessThanOrEqual:
        out << " <= ";
        break;
    case Binary::GreaterThanOrEqual:
        out << " >= ";
        break;
    case Binary::Plus:
        out << " + ";
        break;
    case Binary::Minus:
        out << " - ";
        break;
    case Binary::Multiply:
        out << " * ";
        break;
    case Binary::Divide:
        out << " / ";
        break;
    case Binary::Exponent:
        out << " ^ ";
        break;
    case Binary::Modulo:
        out << " % ";
        break;
    case Binary::Subscript:
        out << "[";
    }
    binary.rightExpression->accept(*this);
    if (binary.binaryOperator == Binary::Subscript) {
        out << "]";
    }
}

void PrettyPrinter::visit(const Unary &e) {
    switch (e.unaryOperator) {
    case Unary::Minus:
        out << "-";
        break;
    case Unary::Not:
        out << "not ";
        break;
    }
    e.expression->accept(*this);
}

void PrettyPrinter::visit(const RangeLiteral &range) {
    if (range.start) {
        range.start->accept(*this);
    }
    out << (range.closed ? "..." : "..<");
    if (range.end) {
        range.end->accept(*this);
    }
}

void PrettyPrinter::visit(const ListLiteral &list) {
    auto it = list.expressions.begin();
    while (it != list.expressions.end()) {
        (*it)->accept(*this);
        it++;
        if (it != list.expressions.end()) {
            out << ", ";
        }
    }
}

void PrettyPrinter::visit(const DictionaryLiteral &dictionary) {
    out << "{";
    auto it = dictionary.values.begin();
    while (it != dictionary.values.end()) {
        it->first->accept(*this);
        out << ": ";
        it->second->accept(*this);
        it++;
        if (it != dictionary.values.end()) {
            out << ", ";
        }
    }
    out << "}";
}

void PrettyPrinter::visit(const Literal &literal) { out << literal.token.text; }

CH_NAMESPACE_END
