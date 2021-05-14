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

#include "Common.h"
#include "ast/Chunk.h"
#include "runtime/Descriptor.h"

#include <string>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

class Interpreter;

struct Container {
    std::vector<Ref<ast::Chunk>> chunkList;
    Owned<Descriptor> descriptor;

    Container(Interpreter &interpreter, const Owned<ast::Expression> &expression);
};

CH_RUNTIME_NAMESPACE_END