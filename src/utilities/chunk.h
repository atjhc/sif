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

#include <iostream>
#include <string>

CH_NAMESPACE_BEGIN

enum type { character, word, item, line };

struct base_chunk {
    base_chunk(type type, std::string &source)
        : _type(type), _begin(source.begin()), _end(source.end()) {}

    template <class T>
    base_chunk(type type, T source) : _type(type), _begin(source.begin()), _end(source.end()) {}

    std::string::iterator begin() { return _begin; }

    std::string::iterator end() { return _end; }

    std::string get() { return std::string(begin(), end()); }

  protected:
    std::string::iterator scan(std::string::iterator it, size_t count);
    std::string::iterator scan_end(std::string::iterator it);

    const type _type;
    std::string::iterator _begin, _end;
};

struct chunk : public base_chunk {
    chunk(type type, size_t location, std::string &source) : base_chunk(type, source) {
        _seek(location);
    }

    chunk(type type, size_t location, const base_chunk &source) : base_chunk(type, source) {
        _seek(location);
    }

  private:
    void _seek(size_t location) {
        _begin = scan(_begin, location);
        _end = scan_end(_begin);
    }
};

struct range_chunk : public base_chunk {
    range_chunk(type type, size_t begin, size_t end, std::string &source)
        : base_chunk(type, source) {
        _seek(begin, end);
    }

    range_chunk(type type, size_t begin, size_t end, const base_chunk &source)
        : base_chunk(type, source) {
        _seek(begin, end);
    }

  private:
    void _seek(size_t begin, size_t end) {
        auto start = _begin;
        _begin = scan(_begin, begin);
        _end = scan_end(scan(start, end));
    }
};

struct random_chunk : public base_chunk {
    random_chunk(type type, const std::function<int(int)> &random, std::string &source)
        : base_chunk(type, source) {
        _seek(random);
    }

    random_chunk(type type, const std::function<int(int)> &random, const base_chunk &source)
        : base_chunk(type, source) {
        _seek(random);
    }

  private:
    void _seek(const std::function<int(int)> &random) {
        int count = 0;
        auto it = _begin;
        while (it < _end) {
            it = scan(it, 1);
            count++;
        }

        int choice = random(count);
        _begin = scan(_begin, choice);
        _end = scan_end(_begin);
    }
};

struct last_chunk : public base_chunk {
    last_chunk(type type, std::string &source) : base_chunk(type, source) { _seek(); }

    last_chunk(type type, const base_chunk &source) : base_chunk(type, source) { _seek(); }

  private:
    void _seek() {
        int count = 0;
        auto it = _begin;
        while (it < _end) {
            it = scan(it, 1);
            count++;
        }

        _begin = scan(_begin, count - 1);
        _end = scan_end(_begin);
    }
};

struct middle_chunk : public base_chunk {
    middle_chunk(type type, std::string &source) : base_chunk(type, source) { _seek(); }

    middle_chunk(type type, const base_chunk &source) : base_chunk(type, source) { _seek(); }

  private:
    void _seek() {
        int count = 0;
        auto it = _begin;
        while (it < _end) {
            it = scan(it, 1);
            count++;
        }

        _begin = scan(_begin, count / 2);
        _end = scan_end(_begin);
    }
};

CH_NAMESPACE_END
