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

void PrettyPrinter::print(const Node &node) { node.accept(*this); }

std::any PrettyPrinter::visitAny(const Program &script) {
    for (auto &handler : script.handlers) {
        handler->accept(*this);
    }
    return std::any();
}

std::any PrettyPrinter::visitAny(const Handler &handler) {
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
        handler.arguments->accept(*this);
    }
    out << std::endl;
    if (handler.statements) {
        handler.statements->accept(*this);
    }
    out << indentString() << "end ";
    handler.messageKey->accept(*this);
    out << std::endl;

    return std::any();
}

std::any PrettyPrinter::visitAny(const StatementList &sl) {
    _indentLevel += 1;
    for (auto &statement : sl.statements) {
        out << indentString();
        statement->accept(*this);
        out << std::endl;
    }
    _indentLevel -= 1;

    return std::any();
}

std::any PrettyPrinter::visitAny(const IdentifierList &il) {
    auto i = il.identifiers.begin();
    while (i != il.identifiers.end()) {
        (*i)->accept(*this);

        i++;
        if (i != il.identifiers.end()) {
            out << ", ";
        }
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const ExpressionList &el) {
    auto i = el.expressions.begin();
    while (i < el.expressions.end()) {
        (*i)->accept(*this);

        i++;
        if (i != el.expressions.end()) {
            out << ", ";
        }
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const If &ifs) {
    out << "if ";
    ifs.condition->accept(*this);
    out << " then" << std::endl;
    ifs.ifStatements->accept(*this);
    if (ifs.elseStatements) {
        out << indentString() << "else" << std::endl;
        ifs.elseStatements->accept(*this);
    }
    out << indentString() << "end if";

    return std::any();
}

std::any PrettyPrinter::visitAny(const Repeat &r) {
    out << "repeat" << std::endl;
    r.statements->accept(*this);
    out << indentString() << "end repeat";

    return std::any();
}

std::any PrettyPrinter::visitAny(const RepeatCount &r) {
    out << "repeat ";
    r.countExpression->accept(*this);
    out << std::endl;
    r.statements->accept(*this);
    out << indentString() << "end repeat";

    return std::any();
}

std::any PrettyPrinter::visitAny(const RepeatRange &r) {
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
    r.statements->accept(*this);
    out << indentString() << "end repeat";

    return std::any();
}

std::any PrettyPrinter::visitAny(const RepeatCondition &r) {
    out << "repeat";
    if (r.conditionValue) {
        out << " while ";
    } else {
        out << " until ";
    }
    r.condition->accept(*this);
    out << std::endl;
    r.statements->accept(*this);
    out << indentString() << "end repeat";

    return std::any();
}

std::any PrettyPrinter::visitAny(const ExitRepeat &) {
    out << "exit repeat";

    return std::any();
}

std::any PrettyPrinter::visitAny(const NextRepeat &) {
    out << "next repeat";

    return std::any();
}

std::any PrettyPrinter::visitAny(const Exit &s) {
    out << "exit ";
    s.messageKey->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Pass &s) {
    out << "pass ";
    s.messageKey->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Global &s) {
    out << "global ";
    s.variables->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Return &s) {
    out << "return ";
    if (s.expression) {
        s.expression->accept(*this);
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const Do &s) {
    out << "do ";
    s.expression->accept(*this);
    if (s.language) {
        out << " as ";
        s.language->accept(*this);
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const Identifier &e) {
    out << e.name;

    return std::any();
}

std::any PrettyPrinter::visitAny(const FunctionCall &e) {
    e.identifier->accept(*this);
    out << "(";
    if (e.arguments) {
        e.arguments->accept(*this);
    }
    out << ")";

    return std::any();
}

std::any PrettyPrinter::visitAny(const Property &p) {
    out << "the ";
    if (p.adjective) {
        p.adjective->accept(*this);
        out << " ";
    }
    p.name->accept(*this);
    if (p.expression) {
        out << " of ";
        p.expression->accept(*this);
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const Descriptor &d) {
    d.name->accept(*this);
    if (d.value) {
        out << " (";
        d.value->accept(*this);
        out << ")";
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const Binary &e) {
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

    return std::any();
}

std::any PrettyPrinter::visitAny(const Logical &e) {
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

    return std::any();
}

std::any PrettyPrinter::visitAny(const Unary &e) {
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

    return std::any();
}

std::any PrettyPrinter::visitAny(const FloatLiteral &f) {
    out << f.value;

    return std::any();
}

std::any PrettyPrinter::visitAny(const IntLiteral &i) {
    out << i.value;

    return std::any();
}

std::any PrettyPrinter::visitAny(const StringLiteral &s) {
    out << "\"" << s.value << "\"";

    return std::any();
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

std::any PrettyPrinter::visitAny(const RangeChunk &c) {
    out << ordinalName(c) << " ";
    c.start->accept(*this);
    if (c.end) {
        out << " to ";
        c.end->accept(*this);
    }
    out << " of ";
    c.expression->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const AnyChunk &c) {
    out << "any " << ordinalName(c) << " of ";
    c.expression->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const LastChunk &c) {
    out << "the last " << ordinalName(c) << " of ";
    c.expression->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const MiddleChunk &c) {
    out << "any " << ordinalName(c) << " of ";
    c.expression->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Command &c) {
    c.name->accept(*this);
    if (c.arguments) {
        c.arguments->accept(*this);
    }

    return std::any();
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

std::any PrettyPrinter::visitAny(const Put &c) {
    out << "put ";
    c.expression->accept(*this);
    if (c.target) {
        out << " " << stringForPreposition(c.preposition) << " ";
        c.target->accept(*this);
    }

    return std::any();
}

std::any PrettyPrinter::visitAny(const Get &c) {
    out << "get ";
    c.expression->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Ask &c) {
    out << "ask ";
    c.expression->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Add &c) {
    out << "add ";
    c.expression->accept(*this);
    out << " to ";
    c.destination->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Subtract &c) {
    out << "subtract ";
    c.expression->accept(*this);
    out << " from ";
    c.destination->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Multiply &c) {
    out << "multiply ";
    c.expression->accept(*this);
    out << " by ";
    c.destination->accept(*this);

    return std::any();
}

std::any PrettyPrinter::visitAny(const Divide &c) {
    out << "divide ";
    c.expression->accept(*this);
    out << " by ";
    c.destination->accept(*this);
    return std::any();
}

CH_AST_NAMESPACE_END
