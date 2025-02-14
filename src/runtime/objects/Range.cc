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

#include "runtime/objects/Range.h"
#include "utilities/hasher.h"

SIF_NAMESPACE_BEGIN

Range::Range(Integer start, Integer end, bool closed) : _start(start), _end(end), _closed(closed) {}

Integer Range::start() const { return _start; }

Integer Range::end() const { return _end; }

bool Range::closed() const { return _closed; }

std::string Range::typeName() const { return "range"; }

std::string Range::description() const {
    std::ostringstream ss;
    ss << _start << ".." << (_closed ? "." : "<") << _end;
    return ss.str();
}

bool Range::equals(Strong<Object> object) const {
    if (const auto &range = Cast<Range>(object)) {
        return _start == range->start() && _end == range->end() && _closed == range->closed();
    }
    return false;
}

size_t Range::hash() const {
    hasher hasher;
    hasher.combine(_start, _end, _closed);
    return hasher.value();
}

bool Range::contains(Integer value) const {
    if (_closed) {
        return value >= _start && value <= _end;
    }
    return value >= _start && value < _end;
}

bool Range::contains(const Range &range) const {
    if (range.closed()) {
        return contains(range.start()) && contains(range.end());
    }
    return contains(range.start()) && contains(range.end() - 1);
}

bool Range::overlaps(const Range &range) const {
    if (range.closed()) {
        return contains(range.start()) || contains(range.end());
    }
    return contains(range.start()) || contains(range.end() - 1);
}

Integer Range::size() const { return (_closed ? 1 : 0) + _end - _start; }

Value Range::enumerator(Value self) const { return MakeStrong<RangeEnumerator>(self.as<Range>()); }

Result<Value, Error> Range::subscript(SourceLocation location, const Value &value) const {
    if (!value.isInteger()) {
        return Fail(Error(location, "expected an integer"));
        return true;
    }
    auto index = value.asInteger();
    if (index >= size() || size() + index < 0) {
        return Fail(Error(location, "range index out of bounds"));
    }
    return Value(start() + index);
}

Result<Value, Error> Range::setSubscript(SourceLocation location, const Value &key, Value value) {
    return Fail(Error(location, "ranges may not be modified"));
}

#pragma mark - RangeEnumerator

RangeEnumerator::RangeEnumerator(Strong<Range> range) : _range(range), _index(0) {}

Value RangeEnumerator::enumerate() {
    if (_index >= _range->size()) {
        return Value();
    }
    Integer value = _range->start() + _index;
    _index++;
    return value;
}

bool RangeEnumerator::isAtEnd() { return _range->size() == _index; }

std::string RangeEnumerator::typeName() const { return "RangeEnumerator"; }

std::string RangeEnumerator::description() const {
    return Concat("E(", _range->description(), ")");
}

SIF_NAMESPACE_END
