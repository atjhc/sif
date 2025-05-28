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

#include "sif/runtime/objects/Dictionary.h"
#include "sif/runtime/objects/List.h"
#include "utilities/hasher.h"

SIF_NAMESPACE_BEGIN

Dictionary::Dictionary() {}

Dictionary::Dictionary(const ValueMap &values) : _values(values) {}

ValueMap &Dictionary::values() { return _values; }

std::string Dictionary::typeName() const { return "dictionary"; }

std::string Dictionary::description() const {
    Set<const Object *> visited;
    return description(visited);
}

std::string Dictionary::description(Set<const Object *> &visited) const {
    if (visited.find(this) != visited.end()) {
        return "[...]";
    }
    visited.insert(this);

    std::ostringstream ss;
    ss << "[";
    auto it = _values.begin();
    while (it != _values.end()) {
        if (it->first.isObject()) {
            ss << it->first.asObject()->description(visited);
        } else {
            ss << it->first.description();
        }

        ss << ": ";
        if (it->second.isObject()) {
            ss << it->second.asObject()->description(visited);
        } else {
            ss << it->second.description();
        }
        it++;
        if (it != _values.end()) {
            ss << ", ";
        }
    }
    if (_values.size() == 0) {
        ss << ":";
    }
    ss << "]";

    // Remove this object from visited set (for other branches)
    visited.erase(this);
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
        h.hash(pair.first, Value::Hash());
        h.hash(pair.second, Value::Hash());
    }
    return h.value();
}

bool Dictionary::contains(const Value &value) const { return _values.find(value) != _values.end(); }

Strong<Object> Dictionary::copy() const { return MakeOwned<Dictionary>(_values); }

Value Dictionary::enumerator(Value self) const {
    return MakeStrong<DictionaryEnumerator>(self.as<Dictionary>());
}

Result<Value, Error> Dictionary::subscript(VirtualMachine &vm, SourceLocation location,
                                           const Value &value) const {
    auto it = _values.find(value);
    if (it == _values.end()) {
        return Value();
    } else {
        return it->second;
    }
}

Result<Value, Error> Dictionary::setSubscript(VirtualMachine &vm, SourceLocation, const Value &key,
                                              Value value) {
    _values[key] = value;
    return Value();
}

void Dictionary::trace(const std::function<void(Strong<Object> &)> &visitor) {
    for (auto &pair : _values) {
        if (pair.first.isObject()) {
            visitor(const_cast<Value &>(pair.first).reference());
        }
        if (pair.second.isObject()) {
            visitor(pair.second.reference());
        }
    }
}

#pragma mark - DictionaryEnumerator

DictionaryEnumerator::DictionaryEnumerator(Strong<Dictionary> dictionary)
    : _dictionary(dictionary), _it(dictionary->values().begin()) {}

Value DictionaryEnumerator::enumerate() {
    auto &&pair = *_it;
    _it++;
    return MakeStrong<List>(std::vector{pair.first, pair.second});
}

bool DictionaryEnumerator::isAtEnd() { return _it == _dictionary->values().end(); }

std::string DictionaryEnumerator::typeName() const { return "DictionaryEnumerator"; }

std::string DictionaryEnumerator::description() const {
    return Concat("E(", _dictionary->description(), ")");
}

void DictionaryEnumerator::trace(const std::function<void(Strong<Object> &)> &visitor) {
    Strong<Object> obj = _dictionary;
    visitor(obj);
}

SIF_NAMESPACE_END
