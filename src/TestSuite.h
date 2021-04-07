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
#include <vector>
#include <string>

CH_NAMESPACE_BEGIN

#define _NS chatter
#define _TEST_FN(GROUP, NAME) _TEST_##GROUP##_##NAME
#define _TEST_DISCARD(GROUP, NAME) _DISCARD_##GROUP##_##NAME

#define TEST_CASE(GROUP, NAME) \
    void _TEST_FN(GROUP, NAME) (_NS::TestSuite &suite); \
    int _TEST_DISCARD(GROUP, NAME) = _NS::MainTestSuite().add(#GROUP, #NAME, _TEST_FN(GROUP, NAME)); \
    void _TEST_FN(GROUP, NAME) (_NS::TestSuite &suite)

#define ASSERT_FAIL(M) suite._assert(false, M, __FILE__, __LINE__)
#define ASSERT_TRUE(C) suite._assert((C), #C " == true", __FILE__, __LINE__)
#define ASSERT_FALSE(C) suite._assert(!(C), #C " == false", __FILE__, __LINE__)
#define ASSERT_NULL(V) suite._assert((V) == nullptr, #V " == nullptr", __FILE__, __LINE__)
#define ASSERT_NOT_NULL(V) suite._assert((V) != nullptr, #V " != nullptr", __FILE__, __LINE__)
#define ASSERT_EQ(LHS, RHS) suite._assert(LHS == RHS, #LHS " == " #RHS, __FILE__, __LINE__)
#define ASSERT_NEQ(LHS, RHS) suite._assert(LHS != RHS, #LHS " != " #RHS, __FILE__, __LINE__)
#define ASSERT_LT(LHS, RHS) suite._assert(LHS < RHS, #LHS " < " #RHS, __FILE__, __LINE__)
#define ASSERT_GT(LHS, RHS) suite._assert(LHS > RHS, #LHS " > " #RHS, __FILE__, __LINE__)
#define ASSERT_LTE(LHS, RHS) suite._assert(LHS <= RHS, #LHS " <= " #RHS, __FILE__, __LINE__)
#define ASSERT_GTE(LHS, RHS) suite._assert(LHS >= RHS, #LHS " >= " #RHS, __FILE__, __LINE__)

struct TestSuite;

struct Test {
    std::string group;
    std::string name;
    std::function<void(TestSuite&)> test;
};

struct TestSuiteConfig {
    std::string resourcesPath;
    std::ostream &out;

    TestSuiteConfig(const std::string &rpath, std::ostream &o)
        : resourcesPath(rpath), out(o) {}

    TestSuiteConfig(const std::string &rpath)
        : resourcesPath(rpath), out(std::cout) {}
};

struct TestSuite {
    TestSuiteConfig config;

    TestSuite(const TestSuiteConfig &c = TestSuiteConfig("src/tests")) : config(c) {}

    int add(const std::string &group, 
            const std::string &name, 
            std::function<void(TestSuite &)> test);

    int runAll();
    int runGroup(const std::string &groupName);
    int runTest(const std::string &groupName, const std::string &testName);

    std::vector<std::string> files_in(const std::string &path) const;
    std::string file_contents(const std::string &path) const;
    std::string basename(const std::string &path) const;
    std::string dirname(const std::string &path) const;

    void _assert(bool condition, const char *test, const char *file, int line);

  private:
    std::vector<Owned<Test>> tests;
    Map<std::string, std::vector<Ref<Test>>> testsByGroup;

    int _summarize();
    void _run(const std::string &name, const std::vector<Ref<Test>> &tests);
    void _run(const Test &test);

    int success_count = 0;
    int failure_count = 0;
};

TestSuite &MainTestSuite();
int RunAllTests();

CH_NAMESPACE_END
