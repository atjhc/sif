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

#include <string>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

class Interpreter;

struct Descriptor {
    std::vector<std::string> names;

    Descriptor(const std::string &name);
    Descriptor(const std::vector<std::string> &names);
    Descriptor(const ast::Descriptor &descriptor);

    std::string description() const;

    bool operator==(const Descriptor &descriptor) const;

    bool is(const std::string &n) const {
        return names.size() == 1 && names[0] == lowercase(n);
    }
};

CH_RUNTIME_NAMESPACE_END

namespace std {

template <>
struct hash<::chatter::runtime::Descriptor> {
    std::size_t operator()(const ::chatter::runtime::Descriptor& d) const {
        return ::chatter::HashRange(d.names);
    }
};

}
