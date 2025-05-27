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

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define _TEST_FN(GROUP, NAME) _TEST_##GROUP##_##NAME
#define _TEST_DISCARD(GROUP, NAME) _DISCARD_##GROUP##_##NAME

#define TEST_CASE(GROUP, NAME)                                                                  \
    void _TEST_FN(GROUP, NAME)(TestSuite & suite);                                              \
    int _TEST_DISCARD(GROUP, NAME) = MainTestSuite().add(#GROUP, #NAME, _TEST_FN(GROUP, NAME)); \
    void _TEST_FN(GROUP, NAME)(TestSuite & suite)

#define ASSERT_FAIL(M) suite._assert(false, M, __FILE__, __LINE__)
#define ASSERT_TRUE(C) suite._assert(!!(C), #C " == true", __FILE__, __LINE__)
#define ASSERT_FALSE(C) suite._assert(!(C), #C " == false", __FILE__, __LINE__)
#define ASSERT_NULL(V) suite._assert((V) == nullptr, #V " == nullptr", __FILE__, __LINE__)
#define ASSERT_NOT_NULL(V) suite._assert((V) != nullptr, #V " != nullptr", __FILE__, __LINE__)
#define ASSERT_EQ(LHS, RHS) suite._assert((LHS) == (RHS), #LHS " == " #RHS, __FILE__, __LINE__)
#define ASSERT_NEQ(LHS, RHS) suite._assert((LHS) != (RHS), #LHS " != " #RHS, __FILE__, __LINE__)
#define ASSERT_LT(LHS, RHS) suite._assert((LHS) < (RHS), #LHS " < " #RHS, __FILE__, __LINE__)
#define ASSERT_GT(LHS, RHS) suite._assert((LHS) > (RHS), #LHS " > " #RHS, __FILE__, __LINE__)
#define ASSERT_LTE(LHS, RHS) suite._assert((LHS) <= (RHS), #LHS " <= " #RHS, __FILE__, __LINE__)
#define ASSERT_GTE(LHS, RHS) suite._assert((LHS) >= (RHS), #LHS " >= " #RHS, __FILE__, __LINE__)
#define ASSERT_NO_THROW(STMNT)                                     \
    ([&]() -> std::ostream & {                                     \
        bool throws = false;                                       \
        try {                                                      \
            (STMNT);                                               \
        } catch (...) {                                            \
            throws = true;                                         \
        }                                                          \
        return suite._assert(!throws, #STMNT, __FILE__, __LINE__); \
    }())
#define ASSERT_THROWS(STMNT)                                      \
    ([&]() -> std::ostream & {                                    \
        bool throws = false;                                      \
        try {                                                     \
            (STMNT);                                              \
        } catch (const std::exception &e) {                       \
            throws = true;                                        \
        }                                                         \
        return suite._assert(throws, #STMNT, __FILE__, __LINE__); \
    }())
#define ASSERT_THROWS_SPECIFIC(STMNT, E)                          \
    ([&]() -> std::ostream & {                                    \
        bool throws = false;                                      \
        try {                                                     \
            (STMNT);                                              \
        } catch (const E &e) {                                    \
            throws = true;                                        \
        }                                                         \
        return suite._assert(throws, #STMNT, __FILE__, __LINE__); \
    }())

struct TestSuite;

struct Test {
    std::string group;
    std::string name;
    std::function<void(TestSuite &)> test;
};

struct TestSuiteConfig {
    std::string resourcesPath;
    std::ostream &out;

    TestSuiteConfig(const std::string &rpath, std::ostream &o) : resourcesPath(rpath), out(o) {}

    TestSuiteConfig(const std::string &rpath) : resourcesPath(rpath), out(std::cout) {}
};

struct TestSuite {
    TestSuiteConfig config;

    TestSuite(const TestSuiteConfig &c = TestSuiteConfig("src/tests/resources")) : config(c) {}

    int run(const std::string &groupName, const std::string &testName);

    std::vector<std::string> files_in(const std::string &path) const;
    std::vector<std::string> all_files_in(const std::string &path) const;
    std::optional<std::string> file_contents(const std::string &path) const;
    std::string basename(const std::string &path) const;
    std::string dirname(const std::string &path) const;

    std::ostream &_assert(bool condition, const char *test, const char *file, int line);

    int add(const std::string &group, const std::string &name,
            std::function<void(TestSuite &)> test);

  private:
    template <class K, class V> using Map = std::unordered_map<K, V>;
    template <class T> using Ref = std::reference_wrapper<T>;
    template <class T> using Owned = std::unique_ptr<T>;

    std::string _currentDateString() const;

    bool _runGroup(const std::string &name, const std::vector<Ref<Test>> &tests,
                   const std::string &testName);
    bool _runTest(const Test &test);

    std::vector<Owned<Test>> tests;
    Map<std::string, std::vector<Ref<Test>>> testsByGroup;

    bool didPass = true;
    int successCount = 0;
    int failureCount = 0;
};

TestSuite &MainTestSuite();
int RunAllTests(int argc, char *argv[]);
