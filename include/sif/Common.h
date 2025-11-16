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

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "extern/expected.hpp"

#define SIF_NAMESPACE_BEGIN namespace sif {
#define SIF_NAMESPACE_END }

SIF_NAMESPACE_BEGIN

class List;
class Dictionary;

#define MAJOR_VERSION 0
#define MINOR_VERSION 0
#define PATCH_VERSION 0

#define XSTR(s) STR(s)
#define STR(s) #s

static const int MajorVersion = MAJOR_VERSION;
static const int MinorVersion = MINOR_VERSION;
static const int PatchVersion = PATCH_VERSION;

[[maybe_unused]] static const char *Version = XSTR(MAJOR_VERSION) "." XSTR(MINOR_VERSION) "." XSTR(PATCH_VERSION);

#undef STR
#undef XSTR
#undef PATCH_VERSION
#undef MINOR_VERSION
#undef MAJOR_VERSION

using Integer = int64_t;
using Bool = bool;
using Float = double;

template <class T, class E> using Result = tl::expected<T, E>;

template <class T> using Strong = std::shared_ptr<T>;

template <class T> using Weak = std::weak_ptr<T>;

template <class T> using Owned = std::unique_ptr<T>;

template <class T> using Set = std::unordered_set<T>;

template <class K, class V, class H = std::hash<K>> using Mapping = std::unordered_map<K, V, H>;

template <class T> using Ref = std::reference_wrapper<T>;

template <class T> using Optional = std::optional<T>;

using NoneType = std::nullopt_t;
inline constexpr NoneType None = std::nullopt;

template <class E> tl::unexpected<E> Fail(const E &error) { return tl::unexpected(error); }

template <class T, class... Args>
std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>> MakeOwned(Args &&...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T, class... Args> std::shared_ptr<T> MakeStrong(Args &&...args) {
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T, class U> Strong<T> Cast(const Strong<U> &arg) {
    return std::dynamic_pointer_cast<T>(arg);
}

template <class T>
inline constexpr bool IsTrackedContainer = std::is_same_v<T, List> || std::is_same_v<T, Dictionary>;

template <typename Iterable>
using ValueType = typename std::iterator_traits<typename Iterable::iterator>::value_type;

struct SourceLocation {
    unsigned int position = 0;
    unsigned int lineNumber = 0;
    unsigned int offset = 0;

    bool operator==(const SourceLocation &location) const {
        return lineNumber == location.lineNumber && position == location.position &&
               offset == location.offset;
    }
    bool operator!=(const SourceLocation &location) const {
        return lineNumber != location.lineNumber || position != location.position ||
               offset != location.offset;
    }
};

struct SourceRange {
    SourceLocation start;
    SourceLocation end;

    bool operator==(const SourceRange &range) const {
        return start == range.start && end == range.end;
    }
    bool operator!=(const SourceRange &range) const {
        return start != range.start || end != range.end;
    }
};

static inline std::ostream &operator<<(std::ostream &out, const SourceLocation &location) {
    return out << (location.lineNumber + 1) << ":" << (location.position + 1);
}

static inline std::ostream &operator<<(std::ostream &out, const SourceRange &range) {
    return out << range.start << "-" << range.end;
}

template <class... Ts> struct Overload : Ts... { using Ts::operator()...; };
template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

SIF_NAMESPACE_END
