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

#include "ast/Command.h"
#include "ast/Identifier.h"

CH_AST_NAMESPACE_BEGIN

Command::Command(Owned<Identifier> &n, Owned<ExpressionList> &args)
    : name(std::move(n)), arguments(std::move(args)) {}

Command::Command(Owned<Identifier> &n, Owned<Expression> &arg)
    : name(std::move(n)), arguments(MakeOwned<ExpressionList>(arg)) {}

Command::Command(Owned<Identifier> &n)
    : name(std::move(n)), arguments(nullptr) {}

Command::Command(Identifier *n)
    : name(n), arguments(nullptr) {}

Put::Put(Owned<Expression> &e, Preposition p, Owned<Identifier> &t)
    : Command(new Identifier("put")), expression(std::move(e)), preposition(p),
        target(std::move(t)) {}

Put::Put(Owned<Expression> &e)
    : Command(new Identifier("put")), expression(std::move(e)), target(nullptr) {}

Get::Get(Owned<Expression> &e) : Command(new Identifier("get")), expression(std::move(e)) {}

Ask::Ask(Owned<Expression> &e) : Command(new Identifier("ask")), expression(std::move(e)) {}

Add::Add(Owned<Expression> &e, Owned<Identifier> &d)
    : Command(new Identifier("add")), expression(std::move(e)), destination(std::move(d)) {}

Subtract::Subtract(Owned<Expression> &e, Owned<Identifier> &d)
    : Command(new Identifier("subtract")), expression(std::move(e)), destination(std::move(d)) {}

Multiply::Multiply(Owned<Expression> &e, Owned<Identifier> &d)
    : Command(new Identifier("multiply")), expression(std::move(e)), destination(std::move(d)) {}

Divide::Divide(Owned<Expression> &e, Owned<Identifier> &d)
    : Command(new Identifier("divide")), expression(std::move(e)), destination(std::move(d)) {}

CH_AST_NAMESPACE_END
