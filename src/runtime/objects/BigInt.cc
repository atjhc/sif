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

#include "sif/runtime/objects/BigInt.h"

#include "extern/bigint.h"

SIF_NAMESPACE_BEGIN

BigInt::BigInt(const std::string &value) : _value(value) {}

const std::string &BigInt::str() const { return _value; }

std::string BigInt::typeName() const { return "integer"; }

std::string BigInt::description() const { return _value; }

std::string BigInt::toString() const { return _value; }

bool BigInt::equals(Strong<Object> object) const {
    if (const auto &other = Cast<BigInt>(object)) {
        return ::BigInt::bigint(_value) == ::BigInt::bigint(other->_value);
    }
    return false;
}

size_t BigInt::hash() const {
    return std::hash<::BigInt::bigint>{}(::BigInt::bigint(_value));
}

Result<Value, Error> BigInt::castFloat() const {
    try {
        return Value(std::stod(_value));
    } catch (...) {
        return Fail(Error("value cannot be represented as a float"));
    }
}

Result<Value, Error> BigInt::castInteger() const {
    try {
        auto val = std::stoll(_value);
        return Value(static_cast<Integer>(val));
    } catch (...) {
        return Fail(Error("value cannot be represented as an integer"));
    }
}

SIF_NAMESPACE_END
