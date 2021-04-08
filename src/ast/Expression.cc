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

#include "Expression.h"

CH_AST_NAMESPACE_BEGIN

ExpressionList::ExpressionList() {}
ExpressionList::ExpressionList(Owned<Expression> &e) { add(e); }

Identifier::Identifier(const std::string &n) : name(n) {}
Identifier::Identifier(const char *n) : name(n) {}

FunctionCall::FunctionCall(Owned<Identifier> &id, Owned<ExpressionList> &args)
    : identifier(std::move(id)), arguments(std::move(args)) {}

FunctionCall::FunctionCall(Owned<Identifier> &id, Owned<Expression> &arg)
    : identifier(std::move(id)), arguments(MakeOwned<ExpressionList>(arg)) {}

FunctionCall::FunctionCall(Owned<Identifier> &id)
    : identifier(std::move(id)), arguments(nullptr) {}

BinaryOp::BinaryOp(Operator o, Owned<Expression> &l, Owned<Expression> &r)
    : op(o), left(std::move(l)), right(std::move(r)) {}

Not::Not(Owned<Expression> &e) : expression(std::move(e)) {}

Minus::Minus(Owned<Expression> &e) : expression(std::move(e)) {}

FloatLiteral::FloatLiteral(float v) : value(v) {}

IntLiteral::IntLiteral(int i) : value(i) {}

StringLiteral::StringLiteral(const std::string &v) : value(v) {}

CH_AST_NAMESPACE_END
