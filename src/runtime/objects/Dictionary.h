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

#include "runtime/protocols/Copyable.h"
#include "runtime/protocols/Subscriptable.h"

#include <string>

SIF_NAMESPACE_BEGIN

class Dictionary : public Object, public Copyable, public Subscriptable {
  public:
    Dictionary();
    Dictionary(const ValueMap &values);

    ValueMap &values();

    bool contains(const Value &value) const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    // Copyable
    Strong<Object> copy() const override;

    // Subscriptable
    Result<Value, RuntimeError> subscript(Location, const Value &) const override;
    Result<Value, RuntimeError> setSubscript(Location, const Value &, Value) override;

  private:
    ValueMap _values;
};

SIF_NAMESPACE_END
