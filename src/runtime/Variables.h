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
#include "Utilities.h"
#include "runtime/Value.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

CH_NAMESPACE_BEGIN

class Variables {
    std::unordered_map<std::string, Value> _values;

public:
    Value get(const std::string &name) const {
        auto key = lowercase(name);
        auto i = _values.find(key);
        if (i != _values.end()) {
            return i->second;
        } else {
            return Value(name);
        }
    }

    void set(const std::string &name, const Value &value) {
        auto key = lowercase(name);
        _values[key] = value;
    }

    void insert(const Variables &variables) {
        _values.insert(variables._values.begin(), variables._values.end());
    }

    void insert(const std::vector<std::string> &names, const std::vector<Value> &values) {
        for (int i = 0; i < names.size(); i++) {
            if (i == values.size()) {
                break;
            }
            set(names[i], values[i]);
        }
    }
};

CH_NAMESPACE_END
