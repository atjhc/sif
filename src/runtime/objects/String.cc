
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

#include "runtime/objects/String.h"

SIF_NAMESPACE_BEGIN

String::String(const std::string &string) : _string(string) {}

const std::string &String::string() const { return _string; }

Value String::operator[](const Range &range) const {
    auto start = _string.begin() + range.start();
    auto end = _string.begin() + range.end() + (range.closed() ? 1 : 0);
    if (start < _string.begin())
        start = _string.begin();
    if (start > _string.end())
        start = _string.end() - 1;
    if (end < _string.begin())
        end = _string.begin();
    if (end > _string.end())
        end = _string.end();
    return MakeStrong<String>(std::string(start, end));
}

std::string String::typeName() const { return "string"; }

std::string String::description() const { return _string; }

bool String::equals(Strong<Object> object) const {
    if (const auto &string = Cast<String>(object)) {
        return _string == string->_string;
    }
    return false;
}

size_t String::hash() const { return std::hash<std::string>{}(_string); }

Strong<Object> String::copy() const { return MakeStrong<String>(_string); }

Value String::enumerator(Value self) const {
    return MakeStrong<StringEnumerator>(self.as<String>());
}

Result<Value, RuntimeError> String::subscript(Location location, Value value) const {
    if (value.isInteger()) {
        auto index = value.asInteger();
        if (index >= _string.size() || _string.size() + index < 0) {
            return Error(RuntimeError(location, Concat("index ", index, " out of bounds")));
            return true;
        }
        return _string.substr(index < 0 ? _string.size() + index : index, 1);
    } else if (auto range = value.as<Range>()) {
        return operator[](*range);
    }
    return Error(RuntimeError(location, "expected an integer or range"));
}

Result<Value, RuntimeError> String::setSubscript(Location location, Value key, Value value) {
    if (auto range = key.as<Range>()) {
        if (auto string = value.as<String>()) {
            _string.replace(_string.begin() + range->start(),
                            _string.begin() + range->end() + (range->closed() ? 1 : 0),
                            string->string());
            return Value();
        }
        return Error(RuntimeError(location, "expected string"));
    }
    if (key.isInteger()) {
        if (auto string = value.as<String>()) {
            _string.replace(_string.begin() + key.asInteger(),
                            _string.begin() + key.asInteger() + 1, string->string());
            return Value();
        }
        return Error(RuntimeError(location, "expected string"));
    }
    return Error(RuntimeError(location, "expected integer or range"));
}

#pragma mark - StringEnumerator

StringEnumerator::StringEnumerator(Strong<String> string) : _string(string), _index(0) {}

Value StringEnumerator::enumerate() {
    if (_index >= _string->string().size()) {
        return Value();
    }
    std::string value = _string->string().substr(_index, 1);
    _index++;
    return value;
}

bool StringEnumerator::isAtEnd() {
    return _string->string().size() == _index;
}

std::string StringEnumerator::typeName() const { return "StringEnumerator"; }

std::string StringEnumerator::description() const {
    return Concat("E(", _string->description(), ")");
}

SIF_NAMESPACE_END
