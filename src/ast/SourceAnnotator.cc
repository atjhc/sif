//
//  Copyright (c) 2025 James Callender
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

#include "SourceAnnotator.h"

SIF_NAMESPACE_BEGIN

SourceAnnotator::SourceAnnotator() {}

std::vector<SourceAnnotator::Annotation>
SourceAnnotator::annotate(const Statement &statement) {
    _ranges.clear();
    statement.accept(*this);
    return _ranges;
}

void SourceAnnotator::visit(const Block &block) {
    for (auto &&statement : block.statements) {
        statement->accept(*this);
    }
}

void SourceAnnotator::visit(const FunctionDecl &functionDecl) {
    _ranges.emplace_back(functionDecl.ranges.function, Annotation:: Kind::Control);
    _ranges.emplace_back(functionDecl.ranges.end, Annotation::Kind::Control);
    if (functionDecl.ranges.endFunction) {
        _ranges.emplace_back(functionDecl.ranges.endFunction.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const If &ifs) {
    _ranges.emplace_back(ifs.ranges.if_, Annotation::Kind::Control);
    ifs.condition->accept(*this);
    _ranges.emplace_back(ifs.ranges.then, Annotation::Kind::Control);
    ifs.ifStatement->accept(*this);
    if (ifs.elseStatement) {
        _ranges.emplace_back(ifs.ranges.else_.value(), Annotation::Kind::Control);
        ifs.elseStatement->accept(*this);
    }
    if (ifs.ranges.end) {
        _ranges.emplace_back(ifs.ranges.end.value(), Annotation::Kind::Control);
    }
    if (ifs.ranges.endIf) {
        _ranges.emplace_back(ifs.ranges.endIf.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const Try &trys) {
    _ranges.emplace_back(trys.ranges.try_, Annotation::Kind::Control);
    trys.statement->accept(*this);
    if (trys.ranges.end) {
        _ranges.emplace_back(trys.ranges.end.value(), Annotation::Kind::Control);
    }
    if (trys.ranges.endTry) {
        _ranges.emplace_back(trys.ranges.endTry.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const Use &use) {
    _ranges.emplace_back(use.ranges.use, Annotation::Kind::Control);
    _ranges.emplace_back(use.target.range, Annotation::Kind::Control);
}

void SourceAnnotator::visit(const Using &usings) {
    _ranges.emplace_back(usings.ranges.using_, Annotation::Kind::Control);
    usings.statement->accept(*this);
    if (usings.ranges.end) {
        _ranges.emplace_back(usings.ranges.end.value(), Annotation::Kind::Control);
    }
    if (usings.ranges.endUsing) {
        _ranges.emplace_back(usings.ranges.endUsing.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const Return &statement) {
    _ranges.emplace_back(statement.ranges.return_, Annotation::Kind::Control);
    if (statement.expression) {
        statement.expression->accept(*this);
    }
}

void SourceAnnotator::visit(const Assignment &set) {
    _ranges.emplace_back(set.ranges.set, Annotation::Kind::Control);
    _ranges.emplace_back(set.ranges.to, Annotation::Kind::Control);
    set.expression->accept(*this);
}

void SourceAnnotator::visit(const ExpressionStatement &statement) {
    statement.expression->accept(*this);
}

void SourceAnnotator::visit(const Repeat &repeat) {
    _ranges.emplace_back(repeat.ranges.repeat, Annotation::Kind::Control);
    repeat.statement->accept(*this);
    _ranges.emplace_back(repeat.ranges.end, Annotation::Kind::Control);
    if (repeat.ranges.endRepeat) {
        _ranges.emplace_back(repeat.ranges.endRepeat.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const RepeatCondition &repeat) {
    _ranges.emplace_back(repeat.Repeat::ranges.repeat, Annotation::Kind::Control);
    _ranges.emplace_back(repeat.ranges.conjunction, Annotation::Kind::Control);
    repeat.condition->accept(*this);
    repeat.statement->accept(*this);
    _ranges.emplace_back(repeat.Repeat::ranges.end, Annotation::Kind::Control);
    if (repeat.Repeat::ranges.endRepeat) {
        _ranges.emplace_back(repeat.Repeat::ranges.endRepeat.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const RepeatFor &foreach) {
    _ranges.emplace_back(foreach.Repeat::ranges.repeat, Annotation::Kind::Control);
    _ranges.emplace_back(foreach.ranges.for_, Annotation::Kind::Control);
    _ranges.emplace_back(foreach.ranges.in, Annotation::Kind::Control);
    foreach.expression->accept(*this);
    _ranges.emplace_back(foreach.Repeat::ranges.end, Annotation::Kind::Control);
    if (foreach.Repeat::ranges.endRepeat) {
        _ranges.emplace_back(foreach.Repeat::ranges.endRepeat.value(), Annotation::Kind::Control);
    }
}

void SourceAnnotator::visit(const ExitRepeat &exitRepeat) {
    _ranges.emplace_back(exitRepeat.ranges.exit, Annotation::Kind::Control);
    _ranges.emplace_back(exitRepeat.ranges.repeat, Annotation::Kind::Control);
}

void SourceAnnotator::visit(const NextRepeat &nextRepeat) {
    _ranges.emplace_back(nextRepeat.ranges.next, Annotation::Kind::Control);
    _ranges.emplace_back(nextRepeat.ranges.repeat, Annotation::Kind::Control);
}

void SourceAnnotator::visit(const Call &call) {
    for (auto &&range : call.ranges) {
        _ranges.emplace_back(range, Annotation::Kind::Call);
    }
    for (auto &&argument : call.arguments) {
        argument->accept(*this);
    }
}

void SourceAnnotator::visit(const Grouping &grouping) {
    _ranges.emplace_back(grouping.ranges.leftGrouping, Annotation::Kind::Operator);
    grouping.expression->accept(*this);
    _ranges.emplace_back(grouping.ranges.rightGrouping, Annotation::Kind::Operator);
}

void SourceAnnotator::visit(const Variable &variable) {
    if (variable.ranges.scope) {
        _ranges.emplace_back(variable.ranges.scope.value(), Annotation::Kind::Operator);
    }
    _ranges.emplace_back(variable.range, Annotation::Kind::Variable);
}

void SourceAnnotator::visit(const Binary &binary) {
    binary.leftExpression->accept(*this);
    _ranges.emplace_back(binary.ranges.operator_, Annotation::Kind::Operator);
    binary.rightExpression->accept(*this);
}

void SourceAnnotator::visit(const Unary &unary) {
    _ranges.emplace_back(unary.ranges.operator_, Annotation::Kind::Operator);
    unary.expression->accept(*this);
}

void SourceAnnotator::visit(const RangeLiteral &range) {
    if (range.start) {
        range.start->accept(*this);
    }
    _ranges.emplace_back(range.ranges.operator_, Annotation::Kind::Operator);
    if (range.end) {
        range.end->accept(*this);
    }
}

void SourceAnnotator::visit(const ListLiteral &list) {
    for (auto &&expression : list.expressions) {
        expression->accept(*this);
    }
}

void SourceAnnotator::visit(const DictionaryLiteral &dictionary) {
    for (auto &&pair : dictionary.values) {
        pair.first->accept(*this);
        pair.second->accept(*this);
    }
}

void SourceAnnotator::visit(const Literal &literal) {
    switch (literal.token.type) {
    case Token::Type::IntLiteral:
    case Token::Type::FloatLiteral:
    case Token::Type::BoolLiteral:
        _ranges.emplace_back(literal.range, Annotation::Kind::NumberLiteral);
        break;
    case Token::Type::StringLiteral:
        _ranges.emplace_back(literal.range, Annotation::Kind::StringLiteral);
    default:
        break;
    }
}

SIF_NAMESPACE_END
