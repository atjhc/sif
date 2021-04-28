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
#include "utilities/chunk.h"

TEST_CASE(ChunkTests, GetChunks) {
    std::string string = "firstly, line 1 of the string\n"
                         "then, line 2\n"
                         "lastly, line 3 of the string\n";

    ASSERT_EQ(chatter::index_chunk(chatter::chunk::character, 0, string).get(), "f");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::character, 5, string).get(), "l");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::character, 100, string).get(), "");

    ASSERT_EQ(chatter::range_chunk(chatter::chunk::character, 0, 6, string).get(), "firstly");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::character, 9, 12, string).get(), "line");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::character, 51, 100, string).get(),
                "line 3 of the string\n");

    ASSERT_EQ(chatter::last_chunk(chatter::chunk::character, string).get(), "\n");

    ASSERT_EQ(chatter::index_chunk(chatter::chunk::word, 0, string).get(), "firstly,");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::word, 5, string).get(), "string");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::word, 100, string).get(), "");

    ASSERT_EQ(chatter::range_chunk(chatter::chunk::word, 0, 5, string).get(),
                "firstly, line 1 of the string");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::word, 6, 7, string).get(), "then, line");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::word, 9, 100, string).get(),
                "lastly, line 3 of the string\n");

    ASSERT_EQ(chatter::last_chunk(chatter::chunk::word, string).get(), "string");

    ASSERT_EQ(chatter::index_chunk(chatter::chunk::item, 0, string).get(), "firstly");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::item, 2, string).get(), " line 2\nlastly");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::item, 100, string).get(), "");

    ASSERT_EQ(chatter::range_chunk(chatter::chunk::item, 0, 1, string).get(),
                "firstly, line 1 of the string\nthen");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::item, 1, 2, string).get(),
                " line 1 of the string\nthen, line 2\nlastly");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::item, 2, 100, string).get(),
                " line 2\nlastly, line 3 of the string\n");

    ASSERT_EQ(chatter::last_chunk(chatter::chunk::item, string).get(), " line 3 of the string\n");

    ASSERT_EQ(chatter::index_chunk(chatter::chunk::line, 0, string).get(), "firstly, line 1 of the string");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::line, 2, string).get(), "lastly, line 3 of the string");
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::line, 100, string).get(), "");

    ASSERT_EQ(chatter::range_chunk(chatter::chunk::line, 0, 1, string).get(),
                "firstly, line 1 of the string\nthen, line 2");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::line, 1, 2, string).get(),
                "then, line 2\nlastly, line 3 of the string");
    ASSERT_EQ(chatter::range_chunk(chatter::chunk::line, 2, 100, string).get(),
                "lastly, line 3 of the string\n");

    ASSERT_EQ(chatter::last_chunk(chatter::chunk::line, string).get(), "lastly, line 3 of the string");

    ASSERT_EQ(chatter::random_chunk(
                    chatter::chunk::line, [](int count) { return 1; }, string)
                    .get(),
                "then, line 2");

    string = "this/string/has/slashes";
    ASSERT_EQ(chatter::index_chunk(chatter::chunk::item, 2, string, '/').get(), "has");
}
