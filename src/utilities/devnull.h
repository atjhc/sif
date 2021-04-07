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

CH_NAMESPACE_BEGIN

template <class cT, class traits = std::char_traits<cT>>
class basic_nullbuf : public std::basic_streambuf<cT, traits> {
    typename traits::int_type overflow(typename traits::int_type c) { return traits::not_eof(c); }
};

template <class cT, class traits = std::char_traits<cT>>
class basic_onullstream : public std::basic_ostream<cT, traits> {
  public:
    basic_onullstream()
        : std::basic_ios<cT, traits>(&_sbuf), std::basic_ostream<cT, traits>(&_sbuf) {
        std::basic_ostream<cT, traits>::init(&_sbuf);
    }

  private:
    basic_nullbuf<cT, traits> _sbuf;
};

template <class cT, class traits = std::char_traits<cT>>
class basic_inullstream : public std::basic_istream<cT, traits> {
  public:
    basic_inullstream()
        : std::basic_ios<cT, traits>(&_sbuf), std::basic_istream<cT, traits>(&_sbuf) {
        std::basic_istream<cT, traits>::init(&_sbuf);
    }

  private:
    basic_nullbuf<cT, traits> _sbuf;
};

typedef basic_onullstream<char> onullstream;
typedef basic_onullstream<wchar_t> wonullstream;
typedef basic_inullstream<char> inullstream;
typedef basic_inullstream<wchar_t> winullstream;

extern onullstream devnull;
extern wonullstream wdevnull;

extern inullstream idevnull;
extern winullstream widevnull;

CH_NAMESPACE_END
