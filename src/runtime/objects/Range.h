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

#include "runtime/protocols/Enumerable.h"
#include "runtime/protocols/Subscriptable.h"

#include <string>

SIF_NAMESPACE_BEGIN

class Range : public Object, public Enumerable, public Subscriptable {
  public:
    Range(Integer start, Integer end, bool closed);

    Integer start() const;
    Integer end() const;
    bool closed() const;
    Integer size() const;

    bool contains(Integer value) const;
    bool contains(const Range &range) const;
    bool overlaps(const Range &range) const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    // Enumerable
    Value enumerator(Value self) const override;

    // Subscriptable
    Result<Value, Error> subscript(Location, const Value &) const override;
    Result<Value, Error> setSubscript(Location, const Value &, Value) override;

  private:
    Integer _start;
    Integer _end;
    bool _closed;
};

class RangeEnumerator : public Enumerator {
  public:
    RangeEnumerator(Strong<Range> range);

    Value enumerate() override;
    bool isAtEnd() override;

    std::string typeName() const override;
    std::string description() const override;

  private:
    Strong<Range> _range;
    size_t _index;
};

SIF_NAMESPACE_END
