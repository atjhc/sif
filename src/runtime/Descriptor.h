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
#include "ast/Descriptor.h"
#include "runtime/Value.h"

#include <vector>
#include <string>

CH_RUNTIME_NAMESPACE_BEGIN

class Interpreter;

struct Descriptor {
    std::vector<std::string> names;
    Optional<Value> value;

    Descriptor(const std::vector<std::string> &n, const Optional<Value> &v);

    // TODO: Move this into Interpreter since it does complex logic.
    Descriptor(Interpreter &r, const ast::Descriptor &d);
};

CH_RUNTIME_NAMESPACE_END
