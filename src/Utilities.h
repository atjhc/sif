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

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

SIF_NAMESPACE_BEGIN

static inline std::string lowercase(const std::string &string) {
    auto result = string;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static inline std::ostream &operator<<(std::ostream &out, const std::vector<std::string> &v) {
    auto i = v.begin();
    while (i != v.end()) {
        out << *i;

        i++;
        if (i != v.end()) {
            out << ", ";
        }
    }
    return out;
}

SIF_NAMESPACE_END
