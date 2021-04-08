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

#include "ast/Property.h"
#include "ast/Identifier.h"

CH_AST_NAMESPACE_BEGIN

Property::Property(Owned<Identifier> &n)
    : adjective(nullptr), name(std::move(n)), expression(nullptr) {}

Property::Property(Owned<Identifier> &adj, Owned<Identifier> &n)
    : adjective(std::move(adj)), name(std::move(n)), expression(nullptr) {}

Property::Property(Owned<Identifier> &n, Owned<Expression> &e)
    : adjective(nullptr), name(std::move(n)), expression(std::move(e)) {}

Property::Property(Owned<Identifier> &adj, Owned<Identifier> &n, Owned<Expression> &e)
    : adjective(std::move(adj)), name(std::move(n)), expression(std::move(e)) {}

CH_AST_NAMESPACE_END
