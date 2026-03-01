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

#include <sif/runtime/protocols/Copyable.h>
#include <sif/runtime/protocols/Enumerable.h>
#include <sif/runtime/protocols/Subscriptable.h>

#include <string>

SIF_NAMESPACE_BEGIN

class VirtualMachine;

class Dictionary : public Object, public Copyable, public Enumerable, public Subscriptable {
  public:
    Dictionary();
    Dictionary(const ValueMap &values);
    Dictionary(ValueMap &&values);

    ValueMap &values();
    const ValueMap &values() const;

    bool contains(const Value &value) const;

    std::string typeName() const override;
    std::string description() const override;
    std::string description(Set<const Object *> &visited) const override;
    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    // Copyable
    Strong<Object> copy(VirtualMachine &vm) const override;

    // Enumerable
    Value enumerator(Value self) const override;

    // Subscriptable
    Result<Value, Error> subscript(VirtualMachine &, SourceLocation, const Value &) const override;
    Result<Value, Error> setSubscript(VirtualMachine &, SourceLocation, const Value &,
                                      Value) override;

    void trace(const std::function<void(Strong<Object> &)> &visitor) override;

  private:
    ValueMap _values;
};

class DictionaryEnumerator : public Enumerator {
  public:
    DictionaryEnumerator(Strong<Dictionary> dictionary);

    Value enumerate() override;
    bool isAtEnd() override;

    std::string typeName() const override;
    std::string description() const override;

    void trace(const std::function<void(Strong<Object> &)> &visitor) override;

  private:
    Dictionary *ptr() const;

    Strong<Object> _dictionary;
    ValueMap::iterator _it;
};

SIF_NAMESPACE_END
