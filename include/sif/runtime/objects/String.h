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
#include <sif/runtime/objects/Range.h>

#include <sif/runtime/protocols/Castable.h>
#include <sif/runtime/protocols/Copyable.h>
#include <sif/runtime/protocols/Enumerable.h>
#include <sif/runtime/protocols/Subscriptable.h>

#include <string>

SIF_NAMESPACE_BEGIN

class String : public Object,
               public Enumerable,
               public Subscriptable,
               public Copyable,
               public NumberCastable {
  public:
    String(const std::string &string);

    std::string &string();
    const std::string &string() const;

    size_t size() const;
    size_t length() const;

    Value operator[](const Range &range) const;

    std::string typeName() const override;

    std::string toString() const override;
    std::string description() const override;
    std::string debugDescription() const override;

    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    void replaceAll(const String &searchString, const String &replacementString);
    void replaceFirst(const String &searchString, const String &replacementString);
    void replaceLast(const String &searchString, const String &replacementString);

    bool contains(const String &searchString) const;
    bool startsWith(const String &searchString) const;
    bool endsWith(const String &searchString) const;

    size_t findFirst(const String &searchString) const;
    size_t findLast(const String &searchString) const;

    // Enumerable
    Value enumerator(Value self) const override;

    // Subscriptable
    Result<Value, Error> subscript(VirtualMachine &, SourceLocation, const Value &) const override;
    Result<Value, Error> setSubscript(VirtualMachine &, SourceLocation, const Value &,
                                      Value) override;

    // Copyable
    Strong<Object> copy() const override;

    // Castable
    Result<Value, Error> castInteger() const override;
    Result<Value, Error> castFloat() const override;

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
