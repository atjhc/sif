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

#include <iostream>
#include <string>

SIF_NAMESPACE_BEGIN

struct chunk {
    enum type { character, word, item, line };

    chunk(type type, const std::string &source)
        : _type(type), _begin(source.cbegin()), _end(source.cend()), _delimiter(",") {}

    chunk(type type, const std::string &source, const std::string &delimiter)
        : _type(type), _begin(source.cbegin()), _end(source.cend()), _delimiter(delimiter) {}

    chunk(const chunk &) = default;

    template <class T>
    chunk(type type, T source)
        : _type(type), _begin(source.begin()), _end(source.end()), _delimiter(",") {}

    template <class T>
    chunk(type type, T source, const std::string &delimiter)
        : _type(type), _begin(source.begin()), _end(source.end()), _delimiter(delimiter) {}

    chunk &operator=(const chunk &) = default;

    std::string::const_iterator begin() { return _begin; }
    std::string::const_iterator end() { return _end; }

    std::string get() { return std::string(begin(), end()); }

    type chunk_type() const { return _type; }

  protected:
    std::string::const_iterator scan(std::string::const_iterator it, size_t count);
    std::string::const_iterator scan_end(std::string::const_iterator it);

    type _type;
    std::string::const_iterator _begin, _end;
    std::string _delimiter;
};

struct index_chunk : public chunk {
    index_chunk(type type, size_t location, const std::string &source) : chunk(type, source) {
        _seek(location);
    }

    index_chunk(type type, size_t location, const std::string &source, const std::string &delimiter)
        : chunk(type, source, delimiter) {
        _seek(location);
    }

    index_chunk(type type, size_t location, const chunk &source) : chunk(type, source) {
        _seek(location);
    }

  private:
    void _seek(size_t location) {
        _begin = scan(_begin, location);
        _end = scan_end(_begin);
    }
};

struct range_chunk : public chunk {
    range_chunk(type type, size_t begin, size_t end, const std::string &source)
        : chunk(type, source) {
        _seek(begin, end);
    }

    range_chunk(type type, size_t begin, size_t end, const std::string &source,
                const std::string &delimiter)
        : chunk(type, source, delimiter) {
        _seek(begin, end);
    }

    range_chunk(type type, size_t begin, size_t end, const chunk &source) : chunk(type, source) {
        _seek(begin, end);
    }

  private:
    void _seek(size_t begin, size_t end) {
        auto start = _begin;
        _begin = scan(_begin, begin);
        _end = scan_end(scan(start, end));
    }
};

template <typename Random> struct random_chunk : public chunk {
    random_chunk(type type, const Random &random, const std::string &source) : chunk(type, source) {
        _seek(random);
    }

    random_chunk(type type, const Random &random, const chunk &source) : chunk(type, source) {
        _seek(random);
    }

  private:
    void _seek(const Random &random) {
        int count = 0;
        auto it = _begin;
        while (it < _end) {
            it = scan(it, 1);
            count++;
        }

        auto choice = random(count);
        _begin = scan(_begin, choice);
        _end = scan_end(_begin);
    }
};

struct last_chunk : public chunk {
    last_chunk(type type, const std::string &source) : chunk(type, source) { _seek(); }

    last_chunk(type type, const chunk &source) : chunk(type, source) { _seek(); }

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

struct middle_chunk : public chunk {
    middle_chunk(type type, const std::string &source) : chunk(type, source) { _seek(); }

    middle_chunk(type type, const chunk &source) : chunk(type, source) { _seek(); }

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

struct count_chunk : public chunk {
    size_t count;

    count_chunk(type type, const std::string &source) : chunk(type, source) { _seek(); }

  private:
    void _seek() {
        count = 0;
        auto it = _begin;
        while (it < _end) {
            it = scan(it, 1);
            count++;
        }
    }
};

SIF_NAMESPACE_END
