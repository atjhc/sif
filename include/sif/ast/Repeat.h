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

#pragma once

#include <sif/Common.h>
#include <sif/ast/Node.h>
#include <sif/ast/Statement.h>

#include <vector>

SIF_NAMESPACE_BEGIN

struct Repeat : Statement {
    Strong<Statement> statement;

    struct {
        SourceRange repeat;
        Optional<SourceRange> forever;
        Optional<SourceRange> end;
        Optional<SourceRange> endRepeat;
    } ranges;

    Repeat() {}
    Repeat(Strong<Statement> statement);

    void accept(Visitor &v) const override { v.visit(*this); }
};

struct RepeatCondition : Repeat {
    Strong<Expression> condition;
    enum Conjunction { While, Until } conjunction;

    struct {
        SourceRange conjunction;
    } ranges;

    RepeatCondition() {}
    RepeatCondition(Strong<Statement> statement, Strong<Expression> condition,
                    Conjunction conjunction);

    void accept(Visitor &v) const override { v.visit(*this); }
};

struct RepeatFor : Repeat {
    std::vector<Strong<Variable>> variables;
    Strong<Expression> expression;

    struct {
        SourceRange for_;
        Optional<SourceRange> in;
    } ranges;

    RepeatFor() {}
    RepeatFor(Strong<Statement> statement, std::vector<Strong<Variable>> variables,
              Strong<Expression> expression);

    void accept(Visitor &v) const override { v.visit(*this); }
};

struct ExitRepeat : Statement {
    struct {
        SourceRange exit;
        Optional<SourceRange> repeat;
    } ranges;

    void accept(Visitor &v) const override { v.visit(*this); }
};

struct NextRepeat : Statement {
    struct {
        SourceRange next;
        Optional<SourceRange> repeat;
    } ranges;

    void accept(Visitor &v) const override { v.visit(*this); }
};

SIF_NAMESPACE_END
