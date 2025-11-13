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

#include "sif/ast/SourceAnnotator.h"

SIF_NAMESPACE_BEGIN

SourceAnnotator::SourceAnnotator() {}

std::vector<Annotation> SourceAnnotator::annotate(const Statement &statement,
                                                  const std::vector<SourceRange> &commentRanges) {
    _annotations.clear();

    // Add comments collected during parsing
    for (const auto &range : commentRanges) {
        _annotations.emplace_back(range, Annotation::Kind::Comment);
    }

    // Traverse AST for all semantic annotations
    statement.accept(*this);

    return _annotations;
}

void SourceAnnotator::visit(const Block &block) {
    for (auto &&statement : block.statements) {
        statement->accept(*this);
    }
}

void SourceAnnotator::visit(const FunctionDecl &functionDecl) {
    _annotations.emplace_back(functionDecl.ranges.function, Annotation::Kind::Keyword);
    if (functionDecl.signature) {
        for (const auto &term : functionDecl.signature.value().terms) {
            auto choice = [&](Signature::Choice choice) {
                for (const auto &token : choice.tokens) {
                    _annotations.emplace_back(token.range, Annotation::Kind::Function);
                }
            };
            std::visit(Overload{
                           [&](Token token) {
                               _annotations.emplace_back(token.range, Annotation::Kind::Function);
                           },
                           [&](Signature::Argument argument) {
                               for (const auto &target : argument.targets) {
                                   if (target.name) {
                                       _annotations.emplace_back(target.name.value().range,
                                                                 Annotation::Kind::Variable);
                                   }
                               }
                           },
                           choice,
                           [&](Signature::Option option) { choice(option.choice); },
                       },
                       term);
        }
    }
    if (functionDecl.statement) {
        functionDecl.statement->accept(*this);
    }
    if (functionDecl.ranges.end) {
        _annotations.emplace_back(functionDecl.ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (functionDecl.ranges.endFunction) {
        _annotations.emplace_back(functionDecl.ranges.endFunction.value(),
                                  Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const If &ifs) {
    _annotations.emplace_back(ifs.ranges.if_, Annotation::Kind::Keyword);
    if (ifs.condition) {
        ifs.condition->accept(*this);
    }
    if (ifs.ranges.then) {
        _annotations.emplace_back(ifs.ranges.then.value(), Annotation::Kind::Keyword);
    }
    if (ifs.ifStatement) {
        ifs.ifStatement->accept(*this);
    }
    if (ifs.ranges.else_) {
        _annotations.emplace_back(ifs.ranges.else_.value(), Annotation::Kind::Keyword);
    }
    if (ifs.elseStatement) {
        ifs.elseStatement->accept(*this);
    }
    if (ifs.ranges.end) {
        _annotations.emplace_back(ifs.ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (ifs.ranges.endIf) {
        _annotations.emplace_back(ifs.ranges.endIf.value(), Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const Try &trys) {
    _annotations.emplace_back(trys.ranges.try_, Annotation::Kind::Keyword);
    if (trys.statement) {
        trys.statement->accept(*this);
    }
    if (trys.ranges.end) {
        _annotations.emplace_back(trys.ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (trys.ranges.endTry) {
        _annotations.emplace_back(trys.ranges.endTry.value(), Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const Use &use) {
    _annotations.emplace_back(use.ranges.use, Annotation::Kind::Keyword);
    _annotations.emplace_back(use.target.range, Annotation::Kind::Namespace);
}

void SourceAnnotator::visit(const Using &usings) {
    _annotations.emplace_back(usings.ranges.using_, Annotation::Kind::Keyword);
    _annotations.emplace_back(usings.target.range, Annotation::Kind::Namespace);
    if (usings.statement) {
        usings.statement->accept(*this);
    }
    if (usings.ranges.end) {
        _annotations.emplace_back(usings.ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (usings.ranges.endUsing) {
        _annotations.emplace_back(usings.ranges.endUsing.value(), Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const Return &statement) {
    _annotations.emplace_back(statement.ranges.return_, Annotation::Kind::Keyword);
    if (statement.expression) {
        statement.expression->accept(*this);
    }
}

void SourceAnnotator::visit(const Assignment &set) {
    _annotations.emplace_back(set.ranges.set, Annotation::Kind::Keyword);
    for (const auto &target : set.targets) {
        _annotations.emplace_back(target->variable->range, Annotation::Kind::Variable);
        for (const auto &subscript : target->subscripts) {
            subscript->accept(*this);
        }
    }
    if (set.ranges.to) {
        _annotations.emplace_back(set.ranges.to.value(), Annotation::Kind::Keyword);
    }
    if (set.expression) {
        set.expression->accept(*this);
    }
}

void SourceAnnotator::visit(const ExpressionStatement &statement) {
    if (statement.expression) {
        statement.expression->accept(*this);
    }
}

void SourceAnnotator::visit(const Repeat &repeat) {
    _annotations.emplace_back(repeat.ranges.repeat, Annotation::Kind::Keyword);
    if (repeat.statement) {
        repeat.statement->accept(*this);
    }
    if (repeat.ranges.end) {
        _annotations.emplace_back(repeat.ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (repeat.ranges.endRepeat) {
        _annotations.emplace_back(repeat.ranges.endRepeat.value(), Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const RepeatCondition &repeat) {
    _annotations.emplace_back(repeat.Repeat::ranges.repeat, Annotation::Kind::Keyword);
    _annotations.emplace_back(repeat.ranges.conjunction, Annotation::Kind::Keyword);
    if (repeat.condition) {
        repeat.condition->accept(*this);
    }
    if (repeat.statement) {
        repeat.statement->accept(*this);
    }
    if (repeat.Repeat::ranges.end) {
        _annotations.emplace_back(repeat.Repeat::ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (repeat.Repeat::ranges.endRepeat) {
        _annotations.emplace_back(repeat.Repeat::ranges.endRepeat.value(),
                                  Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const RepeatFor &foreach) {
    _annotations.emplace_back(foreach.Repeat::ranges.repeat, Annotation::Kind::Keyword);
    _annotations.emplace_back(foreach.ranges.for_, Annotation::Kind::Keyword);
    for (const auto &variable : foreach.variables) {
        variable->accept(*this);
    }
    if (foreach.ranges.in) {
        _annotations.emplace_back(foreach.ranges.in.value(), Annotation::Kind::Keyword);
    }
    if (foreach.expression) {
        foreach.expression->accept(*this);
    }
    if (foreach.statement) {
        foreach.statement->accept(*this);
    }
    if (foreach.Repeat::ranges.end) {
        _annotations.emplace_back(foreach.Repeat::ranges.end.value(), Annotation::Kind::Keyword);
    }
    if (foreach.Repeat::ranges.endRepeat) {
        _annotations.emplace_back(foreach.Repeat::ranges.endRepeat.value(),
                                  Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const ExitRepeat &exitRepeat) {
    _annotations.emplace_back(exitRepeat.ranges.exit, Annotation::Kind::Keyword);
    if (exitRepeat.ranges.repeat) {
        _annotations.emplace_back(exitRepeat.ranges.repeat.value(), Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const NextRepeat &nextRepeat) {
    _annotations.emplace_back(nextRepeat.ranges.next, Annotation::Kind::Keyword);
    if (nextRepeat.ranges.repeat) {
        _annotations.emplace_back(nextRepeat.ranges.repeat.value(), Annotation::Kind::Keyword);
    }
}

void SourceAnnotator::visit(const Call &call) {
    for (auto &&range : call.ranges) {
        _annotations.emplace_back(range, Annotation::Kind::Function);
    }
    for (auto &&argument : call.arguments) {
        argument->accept(*this);
    }
}

void SourceAnnotator::visit(const Grouping &grouping) {
    _annotations.emplace_back(grouping.ranges.leftGrouping, Annotation::Kind::Operator);
    if (grouping.expression) {
        grouping.expression->accept(*this);
    }
    if (grouping.ranges.rightGrouping) {
        _annotations.emplace_back(grouping.ranges.rightGrouping.value(),
                                  Annotation::Kind::Operator);
    }
}

void SourceAnnotator::visit(const Variable &variable) {
    if (variable.ranges.scope) {
        _annotations.emplace_back(variable.ranges.scope.value(), Annotation::Kind::Keyword);
    }
    _annotations.emplace_back(variable.range, Annotation::Kind::Variable);
}

void SourceAnnotator::visit(const Binary &binary) {
    if (binary.leftExpression) {
        binary.leftExpression->accept(*this);
    }
    _annotations.emplace_back(binary.ranges.operator_, Annotation::Kind::Operator);
    if (binary.rightExpression) {
        binary.rightExpression->accept(*this);
    }
}

void SourceAnnotator::visit(const Unary &unary) {
    _annotations.emplace_back(unary.ranges.operator_, Annotation::Kind::Operator);
    if (unary.expression) {
        unary.expression->accept(*this);
    }
}

void SourceAnnotator::visit(const RangeLiteral &range) {
    if (range.start) {
        range.start->accept(*this);
    }
    _annotations.emplace_back(range.ranges.operator_, Annotation::Kind::Operator);
    if (range.end) {
        range.end->accept(*this);
    }
}

void SourceAnnotator::visit(const ListLiteral &list) {
    if (list.ranges.leftBracket) {
        _annotations.emplace_back(list.ranges.leftBracket.value(), Annotation::Kind::Operator);
    }
    for (int i = 0; i < list.expressions.size(); i++) {
        auto expression = list.expressions[i];
        expression->accept(*this);
        if (i < list.ranges.commas.size()) {
            _annotations.emplace_back(list.ranges.commas[i], Annotation::Kind::Operator);
        }
    }
    if (list.ranges.rightBracket) {
        _annotations.emplace_back(list.ranges.rightBracket.value(), Annotation::Kind::Operator);
    }
}

void SourceAnnotator::visit(const DictionaryLiteral &dictionary) {
    _annotations.emplace_back(dictionary.ranges.leftBracket, Annotation::Kind::Operator);
    for (int i = 0; i < dictionary.values.size(); i++) {
        const auto &pair = dictionary.values[i];
        pair.first->accept(*this);
        _annotations.emplace_back(dictionary.ranges.colons[i], Annotation::Kind::Operator);
        pair.second->accept(*this);
        if (i < dictionary.ranges.commas.size()) {
            _annotations.emplace_back(dictionary.ranges.commas[i], Annotation::Kind::Operator);
        }
    }
    if (dictionary.values.size() == 0 && dictionary.ranges.colons.size() == 1) {
        _annotations.emplace_back(dictionary.ranges.colons[0], Annotation::Kind::Operator);
    }
    _annotations.emplace_back(dictionary.ranges.rightBracket, Annotation::Kind::Operator);
}

void SourceAnnotator::visit(const Literal &literal) {
    switch (literal.token.type) {
    case Token::Type::IntLiteral:
    case Token::Type::FloatLiteral:
    case Token::Type::BoolLiteral:
    case Token::Type::Empty:
        _annotations.emplace_back(literal.range, Annotation::Kind::Number);
        break;
    case Token::Type::StringLiteral:
    case Token::Type::OpenInterpolation:
    case Token::Type::Interpolation:
    case Token::Type::ClosedInterpolation:
        _annotations.emplace_back(literal.range, Annotation::Kind::String);
        break;
    default:
        break;
    }
}

void SourceAnnotator::visit(const StringInterpolation &interpolation) {
    _annotations.emplace_back(interpolation.left.range, Annotation::Kind::String);
    if (interpolation.expression) {
        interpolation.expression->accept(*this);
    }
    if (interpolation.right) {
        interpolation.right->accept(*this);
    }
}

SIF_NAMESPACE_END
