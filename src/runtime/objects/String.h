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

#include "Common.h"
#include "runtime/Object.h"
#include "runtime/Value.h"
#include "runtime/objects/Range.h"

#include "runtime/protocols/Copyable.h"
#include "runtime/protocols/Enumerable.h"
#include "runtime/protocols/Subscriptable.h"

#include <string>

SIF_NAMESPACE_BEGIN

class String : public Object, public Enumerable, public Subscriptable, public Copyable {
  public:
    String(const std::string &string);

    const std::string &string() const;

    Value operator[](const Range &range) const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    // Enumerable
    Value enumerator(Value self) const override;

    // Subscriptable
    Result<Value, RuntimeError> subscript(Location location, Value value) const override;
    Result<Value, RuntimeError> setSubscript(Location, Value, Value) override;

    // Copyable
    Strong<Object> copy() const override;

  private:
    std::string _string;
};

class StringEnumerator : public Enumerator {
  public:
    StringEnumerator(Strong<String> list);

    Value enumerate() override;
    bool isAtEnd() override;

    std::string typeName() const override;
    std::string description() const override;

  private:
    Strong<String> _string;
    size_t _index;
};

SIF_NAMESPACE_END
