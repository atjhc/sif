
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

Value String::operator[](int64_t index) const {
    return MakeStrong<String>(_string.substr(index, 1));
}

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
    if (const auto &string = std::dynamic_pointer_cast<String>(object)) {
        return _string == string->_string;
    }
    return false;
}

int64_t String::length() const {
    return _string.size();
}

SIF_NAMESPACE_END
