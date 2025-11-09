
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

#include "sif/runtime/objects/String.h"
#include "sif/runtime/VirtualMachine.h"

#include "extern/utf8.h"
#include "utilities/strings.h"

SIF_NAMESPACE_BEGIN

String::String(const std::string &string) : _string(string) {}

std::string &String::string() { return _string; }

const std::string &String::string() const { return _string; }

size_t String::size() const { return _string.size(); }

size_t String::length() const { return utf8::distance(string().begin(), string().end()); }

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

std::string String::toString() const { return _string; }

std::string String::description() const { return Quoted(_string); }

std::string String::debugDescription() const { return Quoted(escaped_string_from_string(_string)); }

bool String::equals(Strong<Object> object) const {
    if (const auto &string = Cast<String>(object)) {
        return _string == string->_string;
    }
    return false;
}

size_t String::hash() const { return std::hash<std::string>{}(_string); }

Strong<Object> String::copy(VirtualMachine &vm) const { return vm.make<String>(_string); }

void String::replaceAll(const String &searchString, const String &replacementString) {
    size_t offset = 0;
    while (true) {
        auto position = string().find(searchString.string(), offset);
        if (position == std::string::npos) {
            return;
        }
        string().replace(position, searchString.string().size(), replacementString.string());
        offset = position + replacementString.size();
    }
}

void String::replaceFirst(const String &searchString, const String &replacementString) {
    auto position = string().find(searchString.string());
    if (position != std::string::npos) {
        string().replace(position, searchString.string().size(), replacementString.string());
    }
}

void String::replaceLast(const String &searchString, const String &replacementString) {
    auto position = string().rfind(searchString.string());
    if (position != std::string::npos) {
        string().replace(position, searchString.string().size(), replacementString.string());
    }
}

bool String::contains(const String &searchString) const {
    return string().find(searchString.string()) != std::string::npos;
}

bool String::startsWith(const String &searchString) const {
    return string().find(searchString.string()) == 0;
}

bool String::endsWith(const String &searchString) const {
    return string().rfind(searchString.string()) + searchString.size() == size();
}

size_t String::findFirst(const String &searchString) const {
    auto location = string().find(searchString.string());
    if (location == std::string::npos) {
        return location;
    }
    return utf8::distance(string().begin(), string().begin() + location);
}

size_t String::findLast(const String &searchString) const {
    auto location = string().rfind(searchString.string());
    if (location == std::string::npos) {
        return location;
    }
    return utf8::distance(string().begin(), string().begin() + location);
}

Value String::enumerator(Value self) const {
    return MakeStrong<StringEnumerator>(self.as<String>());
}

Result<Value, Error> String::subscript(VirtualMachine &vm, SourceLocation location,
                                       const Value &value) const {
    if (value.isInteger()) {
        auto index = value.asInteger();
        if (index >= _string.size() || _string.size() + index < 0) {
            return Fail(Error(location, Concat("index ", index, " out of bounds")));
            return true;
        }
        return _string.substr(index < 0 ? _string.size() + index : index, 1);
    } else if (auto range = value.as<Range>()) {
        return operator[](*range);
    }
    return Fail(Error(location, "expected an integer or range"));
}

Result<Value, Error> String::setSubscript(VirtualMachine &vm, SourceLocation location,
                                          const Value &key, Value value) {
    if (auto range = key.as<Range>()) {
        if (auto string = value.as<String>()) {
            _string.replace(_string.begin() + range->start(),
                            _string.begin() + range->end() + (range->closed() ? 1 : 0),
                            string->string());
            return Value();
        }
        return Fail(Error(location, "expected string"));
    }
    if (key.isInteger()) {
        if (auto string = value.as<String>()) {
            _string.replace(_string.begin() + key.asInteger(),
                            _string.begin() + key.asInteger() + 1, string->string());
            return Value();
        }
        return Fail(Error(location, "expected string"));
    }
    return Fail(Error(location, "expected integer or range"));
}

#pragma mark - NumberCastable

Result<Value, Error> String::castInteger() const {
    std::stringstream ss(_string);
    Integer number;
    if (ss >> number) {
        return Value(number);
    }
    return Fail(Error("can't convert value to number"));
}

Result<Value, Error> String::castFloat() const {
    std::stringstream ss(_string);
    Float number;
    if (ss >> number) {
        return Value(number);
    }
    return Fail(Error("can't convert value to number"));
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

bool StringEnumerator::isAtEnd() { return _string->string().size() == _index; }

std::string StringEnumerator::typeName() const { return "StringEnumerator"; }

std::string StringEnumerator::description() const {
    return Concat("E(", _string->description(), ")");
}

SIF_NAMESPACE_END
