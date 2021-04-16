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

#include <string>
#include <optional>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

#define CH_NAMESPACE_BEGIN namespace chatter {
#define CH_NAMESPACE_END }

#define CH_AST_NAMESPACE_BEGIN \
    namespace chatter {        \
    namespace ast {
#define CH_AST_NAMESPACE_END \
    }                        \
    }

#define CH_RUNTIME_NAMESPACE_BEGIN \
    namespace chatter {        \
    namespace runtime {
#define CH_RUNTIME_NAMESPACE_END \
    }                        \
    }

CH_NAMESPACE_BEGIN

template <class T>
using Strong = std::shared_ptr<T>;

template <class T>
using Weak = std::weak_ptr<T>;

template <class T>
using Owned = std::unique_ptr<T>;

template <class T>
using Set = std::unordered_set<T>;

template <class K, class V>
using Map = std::unordered_map<K, V>;

template <class T>
using Ref = std::reference_wrapper<T>;

template <class T>
using Optional = std::optional<T>;

inline constexpr std::nullopt_t Empty = std::nullopt;

template<class T, class... Args>
std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>>
MakeOwned(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T, class... Args>
std::shared_ptr<T>
MakeStrong(Args&&... args) {
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}

static inline std::string String() {
    return std::string();
}

template <typename T, typename... Args>
static inline std::string String(T first, Args... args) {
    std::ostringstream ss;
    ss << first << String(args...);
    return ss.str();
}

CH_NAMESPACE_END
