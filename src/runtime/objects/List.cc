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

#include "runtime/objects/List.h"

SIF_NAMESPACE_BEGIN

List::List(const std::vector<Value> &values) : _values(values) {}

std::vector<Value> &List::values() { return _values; }

Value List::operator[](int64_t index) const { return _values[index]; }

Value List::operator[](const Range &range) const {
    auto start = _values.begin();
    auto end = _values.end();
    if (range.start().has_value()) {
        start = start + range.start().value();
        if (start < _values.begin())
            start = _values.begin();
        if (start > _values.end())
            start = _values.end() - 1;
    }
    if (range.end().has_value()) {
        end = _values.begin() + range.end().value() + (range.closed() ? 1 : 0);
        if (end < _values.begin())
            end = _values.begin();
        if (end > _values.end())
            end = _values.end();
    }
    return MakeStrong<List>(std::vector(start, end));
}

std::string List::typeName() const { return "list"; }

std::string List::description() const { return Concat("[", Join(_values, ", "), "]"); }

bool List::equals(Strong<Object> object) const {
    if (const auto &list = std::dynamic_pointer_cast<List>(object)) {
        return _values == list->_values;
    }
    return false;
}

SIF_NAMESPACE_END
