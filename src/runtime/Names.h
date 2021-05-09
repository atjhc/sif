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
#include "ast/Property.h"
#include "ast/Descriptor.h"

#include <string>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

struct Names {
    std::vector<std::string> names;

    Names(const std::string &name);
    Names(const std::string &name1, const std::string &name2);
    Names(const std::vector<std::string> &names);

    Names(const ast::Descriptor &);
    Names(const ast::Property &);
    Names(const ast::FunctionCall &);

    size_t count() const { return names.size(); }
    const std::string &operator[](size_t index) const {
        return names[index];
    }

    const std::string& back() const {
        return names.back();
    }

    const std::string& front() const {
        return names.front();
    }

    std::string description() const;

    bool operator==(const Names &descriptor) const;

    bool is(const std::string &n) const;
    bool is(const std::string &n1, const std::string &n2) const;
};

CH_RUNTIME_NAMESPACE_END

namespace std {

template <>
struct hash<::chatter::runtime::Names> {
    std::size_t operator()(const ::chatter::runtime::Names& d) const {
        return ::chatter::HashRange(d.names);
    }
};

}
