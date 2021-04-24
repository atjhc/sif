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

#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#define CH_NAMESPACE_BEGIN namespace chatter {
#define CH_NAMESPACE_END }

#define CH_AST_NAMESPACE_BEGIN \
    namespace chatter {        \
    namespace ast {
#define CH_AST_NAMESPACE_END \
    }                        \
    }

#define CH_RUNTIME_NAMESPACE_BEGIN \
    namespace chatter {            \
    namespace runtime {
#define CH_RUNTIME_NAMESPACE_END \
    }                            \
    }

CH_NAMESPACE_BEGIN

template <class T> using Strong = std::shared_ptr<T>;

template <class T> using Weak = std::weak_ptr<T>;

template <class T> using Owned = std::unique_ptr<T>;

template <class T> using Set = std::unordered_set<T>;

template <class K, class V> using Map = std::unordered_map<K, V>;

template <class T> using Ref = std::reference_wrapper<T>;

template <class T> using Optional = std::optional<T>;

inline constexpr std::nullopt_t Empty = std::nullopt;

template <class T, class... Args>
std::enable_if_t<!std::is_array<T>::value, std::unique_ptr<T>> MakeOwned(Args &&...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T, class... Args> std::shared_ptr<T> MakeStrong(Args &&...args) {
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}

static inline std::string String() { return std::string(); }

template <typename... Args> 
static inline std::string String(Args... args) {
	std::ostringstream ss;
	(ss << ... << args);
	return ss.str();
}

template <typename T>
class range {
    typename T::iterator _begin;
    typename T::iterator _end;
    
public:
    range(T &i) 
        : _begin(i.begin()), _end(i.end()) {}

    range(typename T::iterator begin, typename T::iterator end) 
        : _begin(begin), _end(end) {}

    typename T::iterator begin() const {
        return _begin;
    }
    
    typename T::iterator end() const {
        return _end;
    }
};

template <typename T>
range<T> make_range(T &x) {
    return range<T>(x);
}

template <typename T>
class reversed_range {
	T &i;
	
public:
	reversed_range(T &i) : i(i) {}
	
	decltype(i.rbegin()) begin() const {
		return i.rbegin();
	}
	
	decltype(i.rend()) end() const {
		return i.rend();
	}
};

template <typename T>
reversed_range<T> reversed(T &x) {
	return reversed_range<T>(x);
}

CH_NAMESPACE_END
