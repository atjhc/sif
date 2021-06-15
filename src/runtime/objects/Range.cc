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

CH_NAMESPACE_BEGIN

Range::Range(Optional<int64_t> start, Optional<int64_t> end, bool closed) 
    : _start(start), _end(end), _closed(closed) {}
    
Optional<int64_t> Range::start() const {
    return _start;
}

Optional<int64_t> Range::end() const {
    return _end;
}

bool Range::closed() const {
    return _closed;
}

std::string Range::typeName() const {
    return "range";
}

std::string Range::description() const {
    std::ostringstream ss;
    if (_start.has_value()) {
        ss << _start.value();
    }
    ss << ".." << (_closed ? "." : "<");
    if (_end.has_value()) {
        ss << _end.value();
    }
    return ss.str();
}

bool Range::equals(Strong<Object> object) const {
    if (const auto &range = std::dynamic_pointer_cast<Range>(object)) {
        return _start == range->start() && _end == range->end() && _closed == range->closed();
    }
    return false;
}

CH_NAMESPACE_END
