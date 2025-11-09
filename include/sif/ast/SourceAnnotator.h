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

#pragma once

#include <sif/Common.h>
#include <sif/ast/Expression.h>
#include <sif/ast/Repeat.h>
#include <sif/ast/Statement.h>

#include <vector>

SIF_NAMESPACE_BEGIN

struct Annotation {
    enum class Kind {
        Keyword,        // Control flow keywords (if, then, else, repeat, function, etc.)
        Function,       // Function names in calls and declarations
        Variable,       // Variable names
        Operator,       // Operators (binary, unary, grouping, etc.)
        String,         // String literals
        Number,         // Number literals (int, float, bool)
        Comment,        // Comments
        Namespace,      // Module/namespace names
    };

    SourceRange range;
    Kind kind;
    uint32_t modifiers = 0;  // Bit flags for modifiers (declaration, readonly, etc.)
};

class SourceAnnotator : public Statement::Visitor, public Expression::Visitor {
  public:
    SourceAnnotator();

    std::vector<Annotation> annotate(const Statement &,
                                      const std::vector<SourceRange> &commentRanges = {});

#pragma mark - Statement::Visitor

    void visit(const Block &) override;
    void visit(const FunctionDecl &) override;
    void visit(const If &) override;
    void visit(const Try &) override;
    void visit(const Use &) override;
    void visit(const Using &) override;
    void visit(const Assignment &) override;
    void visit(const Return &) override;
    void visit(const ExpressionStatement &) override;
    void visit(const Repeat &) override;
    void visit(const RepeatCondition &) override;
    void visit(const RepeatFor &) override;
    void visit(const ExitRepeat &) override;
    void visit(const NextRepeat &) override;

#pragma mark - Expression::Visitor

    void visit(const Call &) override;
    void visit(const Binary &) override;
    void visit(const Unary &) override;
    void visit(const Grouping &) override;
    void visit(const Variable &) override;
    void visit(const RangeLiteral &) override;
    void visit(const ListLiteral &) override;
    void visit(const DictionaryLiteral &) override;
    void visit(const Literal &) override;
    void visit(const StringInterpolation &) override;

  private:
    std::vector<Annotation> _annotations;
};

SIF_NAMESPACE_END
