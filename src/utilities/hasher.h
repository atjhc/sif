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

#include <functional>

SIF_NAMESPACE_BEGIN

class hasher {
  public:
    template <typename T, typename Hasher> void hash(const T &value, const Hasher &hasher) {
        _value = _value ^ (hasher(value) << 1);
    }

    template <typename T> void combine(const T &value) { hash(value, std::hash<T>{}); }

    template <typename Head, typename... Tail>
    void combine(const Head &value, const Tail... remaining) {
        hash(value, std::hash<Head>{});
        combine(remaining...);
    }

    size_t value() const { return _value; }

  private:
    size_t _value = 1;
};

SIF_NAMESPACE_END
