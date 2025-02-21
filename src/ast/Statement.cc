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

#include "ast/Statement.h"

SIF_NAMESPACE_BEGIN

Block::Block(std::vector<Owned<Statement>> statements) : statements(std::move(statements)) {}

FunctionDecl::FunctionDecl(const Signature &signature, Owned<Statement> statement)
    : signature(signature), statement(std::move(statement)) {}

AssignmentTarget::AssignmentTarget(Owned<Variable> variable, Optional<Token> typeName,
                                   std::vector<Owned<Expression>> subscripts)
    : variable(std::move(variable)), typeName(typeName), subscripts(std::move(subscripts)) {}

Assignment::Assignment(std::vector<Owned<AssignmentTarget>> targets, Owned<Expression> expression)
    : targets(std::move(targets)), expression(std::move(expression)) {}

If::If(Owned<Expression> condition, Owned<Statement> ifStatement, Owned<Statement> elseStatement)
    : condition(std::move(condition)), ifStatement(std::move(ifStatement)),
      elseStatement(std::move(elseStatement)) {}

Try::Try(Owned<Statement> statement) : statement(std::move(statement)) {}

Use::Use(Token target) : target(target) {}

Using::Using(Token target, Owned<Statement> statement)
    : target(target), statement(std::move(statement)) {}

Return::Return(Owned<Expression> expression) : expression(std::move(expression)) {}

ExpressionStatement::ExpressionStatement(Owned<Expression> expression)
    : expression(std::move(expression)) {}

SIF_NAMESPACE_END
