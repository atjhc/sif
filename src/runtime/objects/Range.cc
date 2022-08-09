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

SIF_NAMESPACE_BEGIN

Range::Range(int64_t start, int64_t end, bool closed) : _start(start), _end(end), _closed(closed) {}

int64_t Range::start() const { return _start; }

int64_t Range::end() const { return _end; }

bool Range::closed() const { return _closed; }

std::string Range::typeName() const { return "range"; }

std::string Range::description() const {
    std::ostringstream ss;
    ss << _start << ".." << (_closed ? "." : "<") << _end;
    return ss.str();
}

bool Range::equals(Strong<Object> object) const {
    if (const auto &range = std::dynamic_pointer_cast<Range>(object)) {
        return _start == range->start() && _end == range->end() && _closed == range->closed();
    }
    return false;
}

int64_t Range::size() const { return (_closed ? 1 : 0) + _end - _start; }

Value Range::enumerator(Value self) const { return MakeStrong<RangeEnumerator>(self.as<Range>()); }

RangeEnumerator::RangeEnumerator(Strong<Range> range) : _range(range), _index(0) {}

Value RangeEnumerator::enumerate() {
    if (_index >= _range->size()) {
        return Value();
    }
    int64_t value = _range->start() + _index;
    _index++;
    return value;
}

std::string RangeEnumerator::typeName() const { return "RangeEnumerator"; }

std::string RangeEnumerator::description() const {
    return Concat("E(", _range->description(), ")");
}

bool RangeEnumerator::equals(Strong<Object>) const { return false; }

SIF_NAMESPACE_END
