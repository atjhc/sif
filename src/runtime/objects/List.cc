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
#include "utilities/hasher.h"

SIF_NAMESPACE_BEGIN

List::List(const std::vector<Value> &values) : _values(values) {}

std::vector<Value> &List::values() { return _values; }

Value List::operator[](const Range &range) const {
    auto start = _values.begin() + range.start();
    auto end = _values.begin() + range.end() + (range.closed() ? 1 : 0);
    if (start < _values.begin())
        start = _values.begin();
    if (start > _values.end())
        start = _values.end() - 1;
    if (end < _values.begin())
        end = _values.begin();
    if (end > _values.end())
        end = _values.end();
    return MakeStrong<List>(std::vector(start, end));
}

std::string List::typeName() const { return "list"; }

std::string List::description() const { return Concat("{", Join(_values, ", "), "}"); }

bool List::equals(Strong<Object> object) const {
    if (const auto &list = Cast<List>(object)) {
        return _values == list->_values;
    }
    return false;
}

size_t List::hash() const {
    hasher h;
    for (const auto &v : _values) {
        h.hash(v, Value::Hasher());
    }
    return h.value();
}

Value List::enumerator(Value self) const { return MakeStrong<ListEnumerator>(self.as<List>()); }

Result<Value, RuntimeError> List::subscript(Location location, Value value) const {
    if (auto range = value.as<Range>()) {
        return Value(this->operator[](*range));
    }
    if (value.isInteger()) {
        auto index = value.asInteger();
        if (index >= static_cast<int>(_values.size()) ||
            static_cast<int>(_values.size()) + index < 0) {
            return Error(RuntimeError(location, "array index out of bounds"));
        }
        return Value(_values[index < 0 ? _values.size() + index : index]);
    }
    return Error(RuntimeError(location, "expected an integer or range"));
}

Result<Value, RuntimeError> List::setSubscript(Location location, Value key, Value value) {
    if (auto range = key.as<Range>()) {
        _values.erase(_values.begin() + range->start(),
                      _values.begin() + range->end() + (range->closed() ? 1 : 0));
        if (auto list = value.as<List>()) {
            _values.insert(_values.begin(), list->values().begin(), list->values().end());
        } else {
            _values.insert(_values.begin() + range->start(), value);
        }
    }
    if (key.isInteger()) {
        _values[key.asInteger()] = value;
    }
    return Value();
}

#pragma mark - ListEnumerator

ListEnumerator::ListEnumerator(Strong<List> list) : _list(list), _index(0) {}

Value ListEnumerator::enumerate() {
    if (_index >= _list->values().size()) {
        return Value();
    }
    auto value = _list->values()[_index];
    _index++;
    return value;
}

bool ListEnumerator::isAtEnd() { return _list->values().size() == _index; }

std::string ListEnumerator::typeName() const { return "ListEnumerator"; }

std::string ListEnumerator::description() const { return Concat("E(", _list->description(), ")"); }

SIF_NAMESPACE_END
