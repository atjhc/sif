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

#include "runtime/objects/Dictionary.h"
#include "utilities/hasher.h"

SIF_NAMESPACE_BEGIN

Dictionary::Dictionary(const ValueMap &values) : _values(values) {}

ValueMap &Dictionary::values() { return _values; }

std::string Dictionary::typeName() const { return "dictionary"; }

std::string Dictionary::description() const {
    std::ostringstream ss;
    ss << "{";
    auto it = _values.begin();
    while (it != _values.end()) {
        ss << it->first << ": " << it->second;
        it++;
        if (it != _values.end()) {
            ss << ", ";
        }
    }
    ss << "}";
    return ss.str();
}

bool Dictionary::equals(Strong<Object> object) const {
    if (const auto &dictionary = Cast<Dictionary>(object)) {
        return _values == dictionary->_values;
    }
    return false;
}

size_t Dictionary::hash() const {
    hasher h;
    for (const auto &pair : _values) {
        h.hash(pair.first, ValueHasher());
        h.hash(pair.second, ValueHasher());
    }
    return h.value();
}

Result<Value, RuntimeError> Dictionary::subscript(Location location, Value value) const {
    auto it = _values.find(value);
    if (it == _values.end()) {
        return Value();
    } else {
        return it->second;
    }
}

Result<Value, RuntimeError> Dictionary::setSubscript(Location, Value key, Value value) {
    _values[key] = value;
    return Value();
}

SIF_NAMESPACE_END
