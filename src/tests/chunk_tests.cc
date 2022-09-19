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
    std::string string = "最初に, line 1 of the string\n"
                         "thén, line 2\n"
                         "lastly, line 3 of the string\n";

    ASSERT_EQ(sif::index_chunk(sif::chunk::character, 0, string).get(), "最");
    ASSERT_EQ(sif::index_chunk(sif::chunk::character, 5, string).get(), "l");
    ASSERT_EQ(sif::index_chunk(sif::chunk::character, 100, string).get(), "");

    ASSERT_EQ(sif::range_chunk(sif::chunk::character, 0, 2, string).get(), "最初に");
    ASSERT_EQ(sif::range_chunk(sif::chunk::character, 5, 8, string).get(), "line");
    ASSERT_EQ(sif::range_chunk(sif::chunk::character, 39, 100, string).get(),
              "lastly, line 3 of the string\n");

    ASSERT_EQ(sif::last_chunk(sif::chunk::character, string).get(), "\n");

    ASSERT_EQ(sif::index_chunk(sif::chunk::word, 0, string).get(), "最初に,");
    ASSERT_EQ(sif::index_chunk(sif::chunk::word, 5, string).get(), "string");
    ASSERT_EQ(sif::index_chunk(sif::chunk::word, 100, string).get(), "");

    ASSERT_EQ(sif::range_chunk(sif::chunk::word, 0, 5, string).get(),
              "最初に, line 1 of the string");
    ASSERT_EQ(sif::range_chunk(sif::chunk::word, 6, 7, string).get(), "thén, line");
    ASSERT_EQ(sif::range_chunk(sif::chunk::word, 9, 100, string).get(),
              "lastly, line 3 of the string\n");

    ASSERT_EQ(sif::last_chunk(sif::chunk::word, string).get(), "string");

    ASSERT_EQ(sif::index_chunk(sif::chunk::item, 0, string).get(), "最初に");
    ASSERT_EQ(sif::index_chunk(sif::chunk::item, 2, string).get(), " line 2\nlastly");
    ASSERT_EQ(sif::index_chunk(sif::chunk::item, 100, string).get(), "");

    ASSERT_EQ(sif::range_chunk(sif::chunk::item, 0, 1, string).get(),
              "最初に, line 1 of the string\nthén");
    ASSERT_EQ(sif::range_chunk(sif::chunk::item, 1, 2, string).get(),
              " line 1 of the string\nthén, line 2\nlastly");
    ASSERT_EQ(sif::range_chunk(sif::chunk::item, 2, 100, string).get(),
              " line 2\nlastly, line 3 of the string\n");

    ASSERT_EQ(sif::last_chunk(sif::chunk::item, string).get(), " line 3 of the string\n");

    ASSERT_EQ(sif::index_chunk(sif::chunk::line, 0, string).get(), "最初に, line 1 of the string");
    ASSERT_EQ(sif::index_chunk(sif::chunk::line, 2, string).get(), "lastly, line 3 of the string");
    ASSERT_EQ(sif::index_chunk(sif::chunk::line, 100, string).get(), "");

    ASSERT_EQ(sif::range_chunk(sif::chunk::line, 0, 1, string).get(),
              "最初に, line 1 of the string\nthén, line 2");
    ASSERT_EQ(sif::range_chunk(sif::chunk::line, 1, 2, string).get(),
              "thén, line 2\nlastly, line 3 of the string");
    ASSERT_EQ(sif::range_chunk(sif::chunk::line, 2, 100, string).get(),
              "lastly, line 3 of the string\n");

    ASSERT_EQ(sif::last_chunk(sif::chunk::line, string).get(), "lastly, line 3 of the string");

    ASSERT_EQ(sif::random_chunk(
                  sif::chunk::line, [](int count) { return 1; }, string)
                  .get(),
              "thén, line 2");
}
