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
#include "runtime/Value.h"

#include <vector>

CH_NAMESPACE_BEGIN

struct RuntimeMessage {
    std::string name;
    std::vector<Value> arguments;

    RuntimeMessage(const std::string &n, const std::vector<Value> args = {})
        : name(lowercase(n)), arguments(args) {}
};

CH_NAMESPACE_END;
