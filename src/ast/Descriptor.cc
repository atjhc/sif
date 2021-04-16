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

#include "ast/Descriptor.h"
#include "ast/Identifier.h"

CH_AST_NAMESPACE_BEGIN

Descriptor::Descriptor(Owned<Identifier> &n, Owned<Expression> &v)
    : name(std::move(n)), value(std::move(v)) {}

Descriptor::Descriptor(Owned<Identifier> &n) : name(std::move(n)), value(nullptr) {}

CH_AST_NAMESPACE_END
