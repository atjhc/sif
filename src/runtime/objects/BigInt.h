//
//  Copyright (c) 2021 James Callender
//
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

#pragma once

#include <sif/Common.h>
#include <sif/runtime/Object.h>
#include <sif/runtime/Value.h>

#include <sif/runtime/protocols/Castable.h>

#include <sif/runtime/APInt.h>

SIF_NAMESPACE_BEGIN

class BigInt : public Object, public NumberCastable {
  public:
    BigInt(const ::APInt &value);
    BigInt(const std::string &value);
    BigInt(const Value &value);

    std::string typeName() const override;
    std::string description() const override;
    std::string toString() const override;
    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    Result<Value, Error> castFloat() const override;
    Result<Value, Error> castInteger() const override;

    const ::APInt &value() const;

    static Value toValue(const ::APInt &result);
    static double toDouble(const ::APInt &value);
    static Float toFloat(const Value &v);

    static Value add(const Value &lhs, const Value &rhs);
    static Value subtract(const Value &lhs, const Value &rhs);
    static Value multiply(const Value &lhs, const Value &rhs);
    static Value divide(const Value &lhs, const Value &rhs);
    static Value modulo(const Value &lhs, const Value &rhs);
    static Value negate(const Value &v);
    static int compare(const Value &lhs, const Value &rhs);

  private:
    ::APInt _value;
};

SIF_NAMESPACE_END
