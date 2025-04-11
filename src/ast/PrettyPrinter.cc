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

SIF_NAMESPACE_BEGIN

using namespace sif;

PrettyPrinter::PrettyPrinter(const PrettyPrinterConfig &c) : _config(c), out(c.out) {}

void PrettyPrinter::print(const Expression &expression) { expression.accept(*this); }

void PrettyPrinter::print(const Statement &statement) { statement.accept(*this); }

void PrettyPrinter::printBlock(const Statement &statement) {
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
    out << "function";
    if (functionDecl.signature.has_value()) {
        out << " " << functionDecl.signature.value().description();
    }
    printBlock(*functionDecl.statement);
    out << indentString() << "end function";
}

void PrettyPrinter::visit(const If &ifs) {
    out << "if ";
    ifs.condition->accept(*this);
    out << " then";

    printBlock(*ifs.ifStatement);

    if (ifs.elseStatement) {
        out << indentString() << "else";
        printBlock(*ifs.elseStatement);
    }
    out << indentString() << "end if";
}

void PrettyPrinter::visit(const Try &trys) {
    out << "try";
    printBlock(*trys.statement);
    out << indentString() << "end try";
}

void PrettyPrinter::visit(const Use &use) { out << "use " << use.target.description(); }

void PrettyPrinter::visit(const Using &usings) {
    out << "using " << usings.target.description();
    printBlock(*usings.statement);
    out << indentString() << "end using";
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
    auto it = set.targets.begin();
    while (it != set.targets.end()) {
        (*it)->variable->accept(*this);
        if (auto typeName = (*it)->typeName) {
            out << ": " << typeName.value().text;
        }
        for (auto &&subscript : (*it)->subscripts) {
            out << "[";
            subscript->accept(*this);
            out << "]";
        }
        it++;
        if (it != set.targets.end()) {
            out << ", ";
        }
    }

    out << " to ";
    set.expression->accept(*this);
}

void PrettyPrinter::visit(const ExpressionStatement &statement) {
    statement.expression->accept(*this);
}

void PrettyPrinter::visit(const Repeat &repeat) {
    out << "repeat forever";
    printBlock(*repeat.statement);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const RepeatCondition &repeat) {
    out << "repeat";
    if (repeat.conjunction == RepeatCondition::Conjunction::While) {
        out << " while ";
    } else {
        out << " until ";
    }
    repeat.condition->accept(*this);
    printBlock(*repeat.statement);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const RepeatFor &foreach) {
    out << "repeat for ";
    auto it = foreach.variables.begin();
    while (it != foreach.variables.end()) {
        (*it)->accept(*this);
        it++;
        if (it != foreach.variables.end()) {
            out << ", ";
        }
    }
    out << " in ";
    foreach.expression->accept(*this);
    printBlock(*foreach.statement);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const ExitRepeat &) { out << "exit repeat"; }

void PrettyPrinter::visit(const NextRepeat &) { out << "next repeat"; }

void PrettyPrinter::visit(const Call &call) {
    auto argsIt = call.arguments.begin();

    auto it = call.signature.terms.begin();
    while (it < call.signature.terms.end()) {
        const auto &term = *it;
        std::visit(Overload{
                       [&](Token token) { out << token.text; },
                       [&](Signature::Argument argument) {
                           (*argsIt)->accept(*this);
                           argsIt++;
                       },
                       [&](Signature::Choice choice) { out << choice.tokens[0].text; },
                       [&](Signature::Option option) { out << option.choice.tokens[0].text; },
                   },
                   term);
        it++;
        if (it != call.signature.terms.end()) {
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
    if (variable.scope) {
        if (variable.scope.value() == Variable::Scope::Global) {
            out << "global ";
        } else if (variable.scope.value() == Variable::Scope::Local) {
            out << "local ";
        }
    }
    out << variable.name.text;
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

void PrettyPrinter::visit(const StringInterpolation &interpolation) {
    out << interpolation.left.text.substr(0, interpolation.left.text.size());
    // out << "{";
    interpolation.expression->accept(*this);
    // out << "}";

    if (interpolation.right) {
        interpolation.right->accept(*this);
    } else {
        out << "\"";
    }
}

SIF_NAMESPACE_END
