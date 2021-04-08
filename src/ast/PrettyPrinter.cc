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

#include "ast/Script.h"
#include "ast/Command.h"
#include "ast/Property.h"
#include "ast/Descriptor.h"
#include "ast/Chunk.h"
#include "ast/Repeat.h"

CH_AST_NAMESPACE_BEGIN

PrettyPrinter::PrettyPrinter(const PrettyPrinterConfig &c)
    : _config(c), out(c.out) {}

void PrettyPrinter::print(const Node &node) {
    node.accept(*this);
}

std::any PrettyPrinter::visitAny(const Script &script) {
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

std::any PrettyPrinter::visitAny(const Preposition &p) {
    switch (p.type) {
    case Preposition::Before:
        out << "before";
        break;
    case Preposition::Into:
        out << "into";
        break;
    case Preposition::After:
        out << "after";
        break;
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

std::any PrettyPrinter::visitAny(const BinaryOp &e) {
    out << "(";
    e.left->accept(*this);
    switch (e.op) {
    case BinaryOp::Equal:
        out << " = ";
        break;
    case BinaryOp::NotEqual:
        out << " <> ";
        break;
    case BinaryOp::LessThan:
        out << " < ";
        break;
    case BinaryOp::GreaterThan:
        out << " > ";
        break;
    case BinaryOp::LessThanOrEqual:
        out << " <= ";
        break;
    case BinaryOp::GreaterThanOrEqual:
        out << " >= ";
        break;
    case BinaryOp::Plus:
        out << " + ";
        break;
    case BinaryOp::Minus:
        out << " - ";
        break;
    case BinaryOp::Multiply:
        out << " * ";
        break;
    case BinaryOp::Divide:
        out << " / ";
        break;
    case BinaryOp::Exponent:
        out << " ^ ";
        break;
    case BinaryOp::IsIn:
        out << " is in ";
        break;
    case BinaryOp::IsAn:
        out << " is a ";
        break;
    case BinaryOp::Contains:
        out << " contains ";
        break;
    case BinaryOp::Or:
        out << " or ";
        break;
    case BinaryOp::And:
        out << " and ";
        break;
    case BinaryOp::Mod:
        out << " mod ";
        break;
    case BinaryOp::Concat:
        out << " & ";
        break;
    case BinaryOp::ConcatWithSpace:
        out << " && ";
        break;
    }
    e.right->accept(*this);
    out << ")";

	return std::any();
}

std::any PrettyPrinter::visitAny(const Not &n) {
    out << "not (";
    n.expression->accept(*this);
    out << ")";

	return std::any();
}

std::any PrettyPrinter::visitAny(const Minus &m) {
    out << "-(";
    m.expression->accept(*this);
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

std::any PrettyPrinter::visitAny(const Put &c) {
    out << "put ";
    c.expression->accept(*this);
    if (c.preposition) {
        out << " ";
        c.preposition->accept(*this);
    }
    if (c.target) {
        out << " ";
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
