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
#include "runtime/Enumerable.h"
#include "runtime/Object.h"
#include "runtime/Subscriptable.h"
#include "runtime/Value.h"

#include <string>

SIF_NAMESPACE_BEGIN

class Range : public Object, public Enumerable, public Subscriptable {
  public:
    Range(int64_t start, int64_t end, bool closed);

    int64_t start() const;
    int64_t end() const;
    bool closed() const;
    int64_t size() const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;

    // Enumerable
    Value enumerator(Value self) const override;

    // Subscriptable
    Result<Value, RuntimeError> subscript(Location location, Value value) const override;

  private:
    int64_t _start;
    int64_t _end;
    bool _closed;
};

class RangeEnumerator : public Enumerator {
  public:
    RangeEnumerator(Strong<Range> range);

    Value enumerate() override;

    virtual std::string typeName() const override;
    virtual std::string description() const override;
    virtual bool equals(Strong<Object>) const override;

  private:
    Strong<Range> _range;
    size_t _index;
};

SIF_NAMESPACE_END
