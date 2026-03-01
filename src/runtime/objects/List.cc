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

#include "sif/Error.h"
#include "sif/runtime/objects/List.h"
#include "sif/runtime/VirtualMachine.h"

#include "utilities/hasher.h"
#include <sif/Utilities.h>

SIF_NAMESPACE_BEGIN

List::List(const std::vector<Value> &values) : _values(values) {}

List::List(std::vector<Value> &&values) : _values(std::move(values)) {}

std::vector<Value> &List::values() { return _values; }
const std::vector<Value> &List::values() const { return _values; }

size_t List::size() const { return values().size(); }

std::string List::typeName() const { return "list"; }

std::string List::description() const {
    std::unordered_set<const Object *> visited;
    return description(visited);
}

std::string List::description(Set<const Object *> &visited) const {
    if (visited.find(this) != visited.end()) {
        return "[...]";
    }
    visited.insert(this);

    std::ostringstream ss;
    ss << "[";
    auto it = _values.begin();
    while (it != _values.end()) {
        if (it->isObject()) {
            ss << it->asObject()->description(visited);
        } else {
            ss << it->description();
        }
        it++;
        if (it != _values.end()) {
            ss << ", ";
        }
    }
    ss << "]";

    // Remove this object from visited set (for other branches)
    visited.erase(this);
    return ss.str();
}

bool List::equals(Strong<Object> object) const {
    if (const auto &list = Cast<List>(object)) {
        return _values == list->_values;
    }
    return false;
}

size_t List::hash() const {
    hasher h;
    for (const auto &v : _values) {
        h.hash(v, Value::Hash());
    }
    return h.value();
}

void List::replaceAll(const Value &searchValue, const Value &replacementValue) {
    auto result = std::find(_values.begin(), _values.end(), searchValue);
    while (result != _values.end()) {
        *result = replacementValue;
        result = std::find(result + 1, _values.end(), searchValue);
    }
}

void List::replaceFirst(const Value &searchValue, const Value &replacementValue) {
    auto result = std::find(_values.begin(), _values.end(), searchValue);
    if (result != _values.end()) {
        *result = replacementValue;
    }
}

void List::replaceLast(const Value &searchValue, const Value &replacementValue) {
    auto result = std::find(_values.rbegin(), _values.rend(), searchValue);
    if (result != _values.rend()) {
        *result = replacementValue;
    }
}

bool List::contains(const Value &value) const {
    return std::find(_values.begin(), _values.end(), value) != _values.end();
}

bool List::startsWith(const Value &value) const {
    return _values.size() > 0 && _values.front() == value;
}

bool List::endsWith(const Value &value) const {
    return _values.size() > 0 && _values.back() == value;
}

Optional<Integer> List::findFirst(const Value &value) const {
    auto result = std::find(_values.begin(), _values.end(), value);
    if (result == _values.end()) {
        return None;
    }
    return result - _values.begin();
}

Optional<Integer> List::findLast(const Value &value) const {
    auto result = std::find(_values.rbegin(), _values.rend(), value);
    if (result == _values.rend()) {
        return None;
    }
    return result.base() - _values.begin() - 1;
}

Strong<Object> List::copy(VirtualMachine &vm) const { return vm.make<List>(values()); }

Value List::enumerator(Value self) const { return MakeStrong<ListEnumerator>(self.as<List>()); }

Result<Value, Error> List::subscript(VirtualMachine &vm, SourceLocation location,
                                     const Value &value) const {
    if (auto range = value.as<Range>()) {
        auto start = _values.begin() + range->start();
        auto end = _values.begin() + range->end() + (range->closed() ? 1 : 0);
        if (start < _values.begin())
            start = _values.begin();
        if (start > _values.end())
            start = _values.end() - 1;
        if (end < _values.begin())
            end = _values.begin();
        if (end > _values.end())
            end = _values.end();
        return vm.make<List>(std::vector(start, end));
    }
    if (value.isInteger()) {
        auto index = value.asInteger();
        if (index >= static_cast<int>(_values.size()) ||
            static_cast<int>(_values.size()) + index < 0) {
            return Fail(Error(location, Errors::ListIndexOutOfBounds));
        }
        return Value(_values[index < 0 ? _values.size() + index : index]);
    }
    return Fail(Error(location, "expected an integer or range"));
}

Result<Value, Error> List::setSubscript(VirtualMachine &vm, SourceLocation location,
                                        const Value &key, Value value) {
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
        auto index = key.asInteger();
        if (index >= static_cast<Integer>(_values.size()) ||
            static_cast<Integer>(_values.size()) + index < 0) {
            return Fail(Error(location, Errors::ListIndexOutOfBounds));
        }
        _values[index < 0 ? _values.size() + index : index] = value;
    }
    vm.notifyContainerMutation(this);
    return Value();
}

void List::trace(const std::function<void(Strong<Object> &)> &visitor) {
    for (auto &value : _values) {
        if (value.isObject()) {
            visitor(value.reference());
        }
    }
}

#pragma mark - ListEnumerator

ListEnumerator::ListEnumerator(Strong<List> list) : _list(list), _index(0) {}

List *ListEnumerator::ptr() const { return static_cast<List *>(_list.get()); }

Value ListEnumerator::enumerate() {
    if (_index >= ptr()->values().size()) {
        return Value();
    }
    auto value = ptr()->values()[_index];
    _index++;
    return value;
}

bool ListEnumerator::isAtEnd() { return ptr()->values().size() == _index; }

std::string ListEnumerator::typeName() const { return "ListEnumerator"; }

std::string ListEnumerator::description() const { return Concat("E(", ptr()->description(), ")"); }

void ListEnumerator::trace(const std::function<void(Strong<Object> &)> &visitor) {
    visitor(_list);
}

SIF_NAMESPACE_END
