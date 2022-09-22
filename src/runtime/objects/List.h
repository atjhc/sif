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

#include "runtime/protocols/Enumerable.h"
#include "runtime/protocols/Subscriptable.h"

#include <string>

SIF_NAMESPACE_BEGIN

class List : public Object, public Enumerable, public Subscriptable {
  public:
    List(const std::vector<Value> &values = {});
    List(std::vector<Value>::iterator begin, std::vector<Value>::iterator end);

    std::vector<Value> &values();
    const std::vector<Value> &values() const;

    size_t size() const;
    Value operator[](const Range &range) const;

    void replaceAll(const Value &searchValue, const Value &replacementValue);
    void replaceFirst(const Value &searchValue, const Value &replacementValue);
    void replaceLast(const Value &searchValue, const Value &replacementValue);

    bool contains(const Value &value) const;
    bool startsWith(const Value &value) const;
    bool endsWith(const Value &value) const;

    Optional<Integer> findFirst(const Value &value) const;
    Optional<Integer> findLast(const Value &value) const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object> object) const override;
    size_t hash() const override;

    // Enumerable
    Value enumerator(Value self) const override;

    // Subscriptable
    Result<Value, RuntimeError> subscript(Location, const Value &) const override;
    Result<Value, RuntimeError> setSubscript(Location, const Value &, Value) override;

  private:
    std::vector<Value> _values;
};

class ListEnumerator : public Enumerator {
  public:
    ListEnumerator(Strong<List> list);

    Value enumerate() override;
    bool isAtEnd() override;

    std::string typeName() const override;
    std::string description() const override;

  private:
    Strong<List> _list;
    size_t _index;
};

SIF_NAMESPACE_END
