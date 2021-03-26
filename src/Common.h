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

#include <string>

#define CH_NAMESPACE_BEGIN namespace chatter {
#define CH_NAMESPACE_END }

#define CH_AST_NAMESPACE_BEGIN \
    namespace chatter {        \
    namespace ast {
#define CH_AST_NAMESPACE_END \
    }                        \
    }

#define CH_DECL_CLASS_REF(C) \
    class C;                 \
    using C##Ref = std::shared_ptr<C>;
