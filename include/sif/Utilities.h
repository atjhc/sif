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

#include <algorithm>
#include <cstdlib>
#include <format>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

SIF_NAMESPACE_BEGIN

// String formatting
template <class... Args> std::string Format(std::format_string<Args...> fmt, Args &&...args) {
    return std::format(fmt, std::forward<Args>(args)...);
}

// Container utilities
template <typename Container> void Append(Container &target, const Container &source) {
    std::copy(source.begin(), source.end(), std::back_inserter(target));
}

template <typename Iterable, typename Functor>
std::vector<ValueType<Iterable>> Filter(const Iterable &container, Functor f) {
    Iterable result;
    result.reserve(std::distance(container.begin(), container.end()));
    std::copy_if(container.begin(), container.end(), std::back_inserter(result), f);
    return result;
}

template <typename Iterable, typename Functor>
std::vector<ValueType<Iterable>> Filter(Iterable &container, Functor f) {
    Iterable result;
    result.reserve(std::distance(container.begin(), container.end()));
    std::copy_if(container.begin(), container.end(), std::back_inserter(result), f);
    return result;
}

// RAII defer mechanism
template <typename Fn> class DeferGuard {
  public:
    explicit DeferGuard(Fn fn) : _fn(std::move(fn)) {}
    DeferGuard(const DeferGuard &) = delete;
    DeferGuard &operator=(const DeferGuard &) = delete;
    DeferGuard(DeferGuard &&other) noexcept : _fn(std::move(other._fn)), _active(other._active) {
        other._active = false;
    }
    ~DeferGuard() {
        if (_active) {
            _fn();
        }
    }

  private:
    Fn _fn;
    bool _active = true;
};

template <typename Fn> auto Defer(Fn &&fn) {
    return DeferGuard<std::decay_t<Fn>>(std::forward<Fn>(fn));
}

// String utilities
static inline std::string Concat() { return std::string(); }

template <typename... Args> static inline std::string Concat(Args &&...args) {
    std::ostringstream ss;
    (ss << ... << args);
    return ss.str();
}

static inline std::string Quoted(const std::string &str) { return "\"" + str + "\""; }

template <typename Iterable>
static inline std::string Join(const Iterable &v, const std::string &sep) {
    std::ostringstream ss;
    auto it = v.begin();
    while (it != v.end()) {
        ss << *it;
        it++;
        if (it != v.end()) {
            ss << sep;
        }
    }
    return ss.str();
}

template <typename Iterable, typename Functor>
static inline std::string Join(const Iterable &v, const std::string &sep, Functor f) {
    std::ostringstream ss;
    auto it = v.begin();
    while (it != v.end()) {
        ss << f(*it);
        it++;
        if (it != v.end()) {
            ss << sep;
        }
    }
    return ss.str();
}

static inline std::string lowercase(const std::string &string) {
    auto result = string;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Error handling
template <typename... Args> [[noreturn]] static inline void Abort(Args... args) {
    std::cerr << Concat(args...) << std::endl;
    std::abort();
}

// Enum utilities
template <typename T> constexpr typename std::underlying_type<T>::type RawValue(T e) {
    return static_cast<typename std::underlying_type<T>::type>(e);
}

// Stream operators
static inline std::ostream &operator<<(std::ostream &out, const std::vector<std::string> &v) {
    auto i = v.begin();
    while (i != v.end()) {
        out << *i;

        i++;
        if (i != v.end()) {
            out << ", ";
        }
    }
    return out;
}

SIF_NAMESPACE_END
