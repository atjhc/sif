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

#include "ast/Chunk.h"
#include "ast/Command.h"
#include "ast/Descriptor.h"
#include "ast/Program.h"
#include "ast/Property.h"
#include "ast/Repeat.h"

CH_AST_NAMESPACE_BEGIN

PrettyPrinter::PrettyPrinter(const PrettyPrinterConfig &c) : _config(c), out(c.out) {}

void PrettyPrinter::print(const Program &script) {
    for (auto &handler : script.handlers) {
        print(*handler);
    }
}

void PrettyPrinter::print(const Handler &handler) {
    switch (handler.kind) {
    case Handler::HandlerKind:
        out << indentString() << "on ";
        break;
    case Handler::FunctionKind:
        out << indentString() << "function ";
        break;
    }

    handler.messageKey->accept(*this);
    if (handler.arguments) {
        out << " ";
        print(*handler.arguments, ", ");
    }
    out << std::endl;
    if (handler.statements) {
        print(*handler.statements);
    }
    out << indentString() << "end ";
    handler.messageKey->accept(*this);
    out << std::endl;
}

void PrettyPrinter::print(const StatementList &sl) {
    _indentLevel += 1;
    for (auto &statement : sl.statements) {
        out << indentString();
        statement->accept(*this);
        out << std::endl;
    }
    _indentLevel -= 1;
}

void PrettyPrinter::print(const IdentifierList &il, const std::string &sep) {
    auto i = il.identifiers.begin();
    while (i != il.identifiers.end()) {
        (*i)->accept(*this);

        i++;
        if (i != il.identifiers.end()) {
            out << sep;
        }
    }
}

void PrettyPrinter::print(const ExpressionList &el) {
    auto i = el.expressions.begin();
    while (i < el.expressions.end()) {
        (*i)->accept(*this);

        i++;
        if (i != el.expressions.end()) {
            out << ", ";
        }
    }
}

void PrettyPrinter::visit(const If &ifs) {
    out << "if ";
    ifs.condition->accept(*this);
    out << " then" << std::endl;
    print(*ifs.ifStatements);
    if (ifs.elseStatements) {
        out << indentString() << "else" << std::endl;
        print(*ifs.elseStatements);
    }
    out << indentString() << "end if";
}

void PrettyPrinter::visit(const Repeat &r) {
    out << "repeat" << std::endl;
    print(*r.statements);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const RepeatCount &r) {
    out << "repeat ";
    r.countExpression->accept(*this);
    out << std::endl;
    print(*r.statements);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const RepeatRange &r) {
    out << "repeat with ";
    r.variable->accept(*this);
    out << " = ";
    r.startExpression->accept(*this);
    if (r.ascending) {
        out << " to ";
    } else {
        out << " down to ";
    }
    r.endExpression->accept(*this);
    out << std::endl;
    print(*r.statements);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const RepeatCondition &r) {
    out << "repeat";
    if (r.conditionValue) {
        out << " while ";
    } else {
        out << " until ";
    }
    r.condition->accept(*this);
    out << std::endl;
    print(*r.statements);
    out << indentString() << "end repeat";
}

void PrettyPrinter::visit(const ExitRepeat &) {
    out << "exit repeat";
}

void PrettyPrinter::visit(const NextRepeat &) {
    out << "next repeat";
}

void PrettyPrinter::visit(const Exit &s) {
    out << "exit ";
    s.messageKey->accept(*this);
}

void PrettyPrinter::visit(const Pass &s) {
    out << "pass ";
    s.messageKey->accept(*this);
}

void PrettyPrinter::visit(const Global &s) {
    out << "global ";
    print(*s.variables, ", ");
}

void PrettyPrinter::visit(const Return &s) {
    out << "return ";
    if (s.expression) {
        s.expression->accept(*this);
    }
}

void PrettyPrinter::visit(const Do &s) {
    out << "do ";
    s.expression->accept(*this);
    if (s.language) {
        out << " as ";
        s.language->accept(*this);
    }
}

void PrettyPrinter::visit(const Identifier &e) {
    out << e.name;
}

void PrettyPrinter::visit(const FunctionCall &e) {
    e.name->accept(*this);
    out << "(";
    if (e.arguments) {
        print(*e.arguments);
    }
    out << ")";
}

void PrettyPrinter::visit(const Property &p) {
    out << "the ";
    print(*p.identifiers, " ");
    if (p.expression) {
        out << " of ";
        p.expression->accept(*this);
    }
}

void PrettyPrinter::visit(const Descriptor &d) {
    print(*d.identifiers, " ");
    if (d.value) {
        out << " ";
        d.value->accept(*this);
    }
}

void PrettyPrinter::visit(const Binary &e) {
    out << "(";
    e.leftExpression->accept(*this);
    switch (e.binaryOperator) {
    case Binary::Equal:
        out << " = ";
        break;
    case Binary::NotEqual:
        out << " <> ";
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
    case Binary::IsIn:
        out << " is in ";
        break;
    case Binary::IsA:
        out << " is a ";
        break;
    case Binary::Contains:
        out << " contains ";
        break;
    case Binary::Mod:
        out << " mod ";
        break;
    case Binary::Concat:
        out << " & ";
        break;
    case Binary::ConcatWithSpace:
        out << " && ";
        break;
    }
    e.rightExpression->accept(*this);
    out << ")";
}

void PrettyPrinter::visit(const Logical &e) {
    out << "(";
    e.leftExpression->accept(*this);
    switch (e.logicalOperator) {
    case Logical::Or:
        out << " or ";
        break;
    case Logical::And:
        out << " and ";
        break;
    }
    e.rightExpression->accept(*this);
    out << ")";
}

void PrettyPrinter::visit(const Unary &e) {
    switch (e.unaryOperator) {
    case Unary::ThereIsA:
        out << "there is a";
        break;
    case Unary::Minus:
        out << "-";
        break;
    case Unary::Not:
        out << "not";
        break;
    }

    out << " (";
    e.expression->accept(*this);
    out << ")";
}

void PrettyPrinter::visit(const FloatLiteral &f) {
    out << f.value;
}

void PrettyPrinter::visit(const IntLiteral &i) {
    out << i.value;
}

void PrettyPrinter::visit(const StringLiteral &s) {
    out << "\"" << s.value << "\"";
}

void PrettyPrinter::visit(const ChunkExpression &e) {
    e.chunk->accept(*this);
    out << " of ";
    e.expression->accept(*this);
}

static std::string ordinalName(const Chunk &c) {
    switch (c.type) {
    case Chunk::Char:
        return "char";
    case Chunk::Word:
        return "word";
    case Chunk::Item:
        return "item";
    case Chunk::Line:
        return "line";
    }
}

void PrettyPrinter::visit(const RangeChunk &c) {
    out << ordinalName(c) << " ";
    c.start->accept(*this);
    if (c.end) {
        out << " to ";
        c.end->accept(*this);
    }
}

void PrettyPrinter::visit(const AnyChunk &c) {
    out << "any " << ordinalName(c);
}

void PrettyPrinter::visit(const LastChunk &c) {
    out << "the last " << ordinalName(c);
}

void PrettyPrinter::visit(const MiddleChunk &c) {
    out << "the middle " << ordinalName(c);
}

void PrettyPrinter::visit(const Command &c) {
    c.name->accept(*this);
    if (c.arguments) {
        print(*c.arguments);
    }
}

std::string stringForPreposition(const Put::Preposition &p) {
    switch (p) {
    case Put::Before:
        return "before";
    case Put::Into:
        return "into";
    case Put::After:
        return "after";
    }
}

void PrettyPrinter::visit(const Put &c) {
    out << "put ";
    c.expression->accept(*this);
    if (c.target) {
        out << " " << stringForPreposition(c.preposition) << " ";
        c.target->accept(*this);
    }
}

void PrettyPrinter::visit(const Get &c) {
    out << "get ";
    c.expression->accept(*this);
}

void PrettyPrinter::visit(const Set &c) {
    out << "set ";
    c.property->accept(*this);
    out << " to ";
    c.expression->accept(*this);
}

void PrettyPrinter::visit(const Ask &c) {
    out << "ask ";
    c.expression->accept(*this);
}

void PrettyPrinter::visit(const Add &c) {
    out << "add ";
    c.expression->accept(*this);
    out << " to ";
    c.container->accept(*this);
}

void PrettyPrinter::visit(const Subtract &c) {
    out << "subtract ";
    c.expression->accept(*this);
    out << " from ";
    c.container->accept(*this);
}

void PrettyPrinter::visit(const Multiply &c) {
    out << "multiply ";
    c.expression->accept(*this);
    out << " by ";
    c.container->accept(*this);
}

void PrettyPrinter::visit(const Divide &c) {
    out << "divide ";
    c.expression->accept(*this);
    out << " by ";
    c.container->accept(*this);
}

void PrettyPrinter::visit(const Delete &c) {
    out << "delete ";
    c.expression->accept(*this);
}

CH_AST_NAMESPACE_END
