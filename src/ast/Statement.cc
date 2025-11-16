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

#include <sif/ast/Statement.h>

SIF_NAMESPACE_BEGIN

Block::Block(std::vector<Strong<Statement>> statements) : statements(statements) {}

FunctionDecl::FunctionDecl(const Signature &signature, Strong<Statement> statement)
    : signature(signature), statement(statement) {}

VariableTarget::VariableTarget(Strong<Variable> variable, Optional<Token> typeName,
                               std::vector<Strong<Expression>> subscripts)
    : variable(variable), typeName(typeName), subscripts(subscripts) {
    range = variable->range;
}

void VariableTarget::accept(Visitor &visitor) const { visitor.visit(*this); }

StructuredTarget::StructuredTarget(std::vector<Strong<AssignmentTarget>> targets) : targets(targets) {
    if (!targets.empty()) {
        range = SourceRange{targets.front()->range.start, targets.back()->range.end};
    }
}

void StructuredTarget::accept(Visitor &visitor) const { visitor.visit(*this); }

Assignment::Assignment(std::vector<Strong<AssignmentTarget>> targets, Strong<Expression> expression)
    : targets(targets), expression(expression) {}

If::If(Strong<Expression> condition, Strong<Statement> ifStatement, Strong<Statement> elseStatement)
    : condition(condition), ifStatement(ifStatement), elseStatement(elseStatement) {}

Try::Try(Strong<Statement> statement) : statement(statement) {}

Use::Use(Token target) : target(target) {}

Using::Using(Token target, Strong<Statement> statement) : target(target), statement(statement) {}

Return::Return(Strong<Expression> expression) : expression(expression) {}

ExpressionStatement::ExpressionStatement(Strong<Expression> expression) : expression(expression) {}

SIF_NAMESPACE_END
