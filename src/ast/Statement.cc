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
#include "ast/Identifier.h"

CH_AST_NAMESPACE_BEGIN

StatementList::StatementList() {}
StatementList::StatementList(Owned<Statement> &statement) { add(statement); }

If::If(Owned<Expression> &c, Owned<StatementList> &is, Owned<StatementList> &es)
    : condition(std::move(c)), ifStatements(std::move(is)), elseStatements(std::move(es)) {}

If::If(Owned<Expression> &c, Owned<StatementList> &isl)
    : condition(std::move(c)), ifStatements(std::move(isl)), elseStatements(nullptr) {}

If::If(Owned<Expression> &c, Owned<Statement> &is)
    : condition(std::move(c)), ifStatements(MakeOwned<StatementList>(is)), elseStatements(nullptr) {}

If::If(Owned<Expression> &c, Owned<Statement> &is, Owned<Statement> &es)
    : condition(std::move(c)), ifStatements(MakeOwned<StatementList>(is)), elseStatements(MakeOwned<StatementList>(es)) {}

If::If(Owned<Expression> &c, Owned<Statement> &is, Owned<StatementList> &esl)
    : condition(std::move(c)), ifStatements(MakeOwned<StatementList>(is)), elseStatements(std::move(esl)) {}

Exit::Exit(Owned<Identifier> &m) : messageKey(std::move(m)) {}

Pass::Pass(Owned<Identifier> &m) : messageKey(std::move(m)) {}

Global::Global(Owned<IdentifierList> &v) : variables(std::move(v)) {}

Return::Return(Owned<Expression> &e) : expression(std::move(e)) {}

Do::Do(Owned<Expression> &e) : expression(std::move(e)), language(nullptr) {}
Do::Do(Owned<Expression> &e, Owned<Expression> &l)
    : expression(std::move(e)), language(std::move(l)) {}

CH_AST_NAMESPACE_END
