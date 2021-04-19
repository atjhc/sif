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

#include <any>
#include <ostream>

CH_AST_NAMESPACE_BEGIN

struct Location {
    unsigned int position = 1;
    unsigned int lineNumber = 1;
};

struct Node {
    Location location;

    virtual ~Node() = default;
};

static inline std::ostream &operator<<(std::ostream &out, const Location &location) {
    return out << location.lineNumber << ":" << location.position;
}

CH_AST_NAMESPACE_END
