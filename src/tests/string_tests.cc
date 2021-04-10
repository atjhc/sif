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

#include "tests/TestSuite.h"
#include "utilities/strings.h"

TEST_CASE(Strings, Escape) {
    auto escape = chatter::string_from_escaped_string;

    ASSERT_EQ(escape("Hello, World!"), "Hello, World!");
    ASSERT_EQ(escape("Hello\\n World!"), "Hello\n World!");
    ASSERT_EQ(escape("Hello\\\', World!"), "Hello\', World!");
    ASSERT_EQ(escape("Hello\\\", World!"), "Hello\", World!");
    ASSERT_EQ(escape("Hello\\\\, World!"), "Hello\\, World!");
    ASSERT_EQ(escape("Hello\\f, World!"), "Hello\f, World!");
    ASSERT_EQ(escape("Hello\\r, World!"), "Hello\r, World!");
    ASSERT_EQ(escape("Hello\\t, World!"), "Hello\t, World!");
    ASSERT_EQ(escape("Hello\\v, World!"), "Hello\v, World!");
    ASSERT_EQ(escape("Hello, World!\\040"), "Hello, World!\040");
    ASSERT_EQ(escape("Hello, World!\\40"), "Hello, World!\40");
    ASSERT_EQ(escape("Hello\\x32, World!"), "Hello\x32, World!");
}
