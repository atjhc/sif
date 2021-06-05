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

#include "runtime/objects/Native.h"

CH_NAMESPACE_BEGIN

Native::Native(size_t arity, const Native::Callable &callable)
    : _arity(arity), _callable(callable) {}

size_t Native::arity() const {
    return _arity;
}

const Native::Native::Callable &Native::callable() const {
    return _callable;
}

std::string Native::typeName() const {
    return "function";
}

std::string Native::description() const {
    return "<native function>";
}

bool Native::equals(Strong<Object> object) const {
    return this == object.get();
}

CH_NAMESPACE_END
