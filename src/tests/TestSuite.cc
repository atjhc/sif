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

#include "TestSuite.h"

#include <getopt.h>
#include <sys/types.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#undef basename

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

static basic_onullstream<char> devnull;

TestSuite &MainTestSuite() {
    static TestSuite mainTestSuite;
    return mainTestSuite;
}

static int usage(int argc, char *argv[]) {
    auto basename = std::filesystem::path(argv[0]).filename();
    std::cout << "Usage: " << basename << " [options...] [file]" << std::endl
              << " -t, --test"
              << "\t Run a specific test, requires -g" << std::endl
              << " -g, --group"
              << "\t Specify a group to test" << std::endl
              << " -h, --help"
              << "\t Print out this help and exit" << std::endl;
    return -1;
}

int RunAllTests(int argc, char *argv[]) {
    static struct option long_options[] = {{"group", required_argument, NULL, 'g'},
                                           {"test", required_argument, NULL, 't'},
                                           {"help", no_argument, NULL, 'h'},
                                           {0, 0, 0, 0}};

    std::string groupName;
    std::string testName;

    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "g:t:h", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'g':
            groupName = optarg;
            break;
        case 't':
            testName = optarg;
            break;
        case 'h':
            return usage(argc, argv);
        default:
            break;
        }
    }

    if (!testName.empty() && groupName.empty()) {
        std::cerr << "Requires group name" << std::endl;
        return usage(argc, argv);
    }

    return MainTestSuite().run(groupName, testName);
}

int TestSuite::run(const std::string &groupName, const std::string &testName) {
    auto start = std::chrono::steady_clock::now();

    for (auto &group : testsByGroup) {
        if (!groupName.empty() && groupName != group.first) {
            continue;
        }
        _runGroup(group.first, group.second, testName);
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsedSeconds = end - start;

    config.out << "\tExecuted " << failureCount + successCount << " tests with, " << failureCount
               << (failureCount == 1 ? " failure in " : " failures in ")
               << std::setiosflags(std::ios::fixed) << std::setprecision(5)
               << elapsedSeconds.count() << " seconds." << std::endl;
    return failureCount;
}

bool TestSuite::_runGroup(const std::string &name, const std::vector<Ref<Test>> &tests,
                          const std::string &testName) {
    config.out << "Test Group '" << name << "' started at " << _currentDateString() << std::endl;
    bool passed = true;

    for (auto &test : tests) {
        if (!testName.empty() && testName != test.get().name) {
            continue;
        }
        passed = _runTest(test.get()) && passed;
    }
    config.out << "Test Group '" << name << "' " << (passed ? "passed" : "failed") << " at "
               << _currentDateString() << std::endl;

    return passed;
}

bool TestSuite::_runTest(const Test &test) {
    config.out << "Test Case '" << test.group << "." << test.name << "' started." << std::endl;
    bool passed = true;

    auto start = std::chrono::steady_clock::now();
    test.test(*this);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsedSeconds = end - start;

    passed = didPass;
    didPass = true;

    config.out << "Test Case '" << test.group << "." << test.name << "' "
               << (passed ? "passed" : "failed") << " (" << std::setiosflags(std::ios::fixed)
               << std::setprecision(5) << elapsedSeconds.count() << " seconds)." << std::endl;

    if (passed) {
        successCount++;
    } else {
        failureCount++;
    }

    return passed;
}
std::string TestSuite::_currentDateString() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

std::ostream &TestSuite::_assert(bool condition, const char *test, const char *file, int line) {
    if (condition) {
        return devnull;
    } else {
        didPass = false;
        config.out << file << ":" << line << ": error: \"" << test << "\" failed." << std::endl;
        return config.out;
    }
}

int TestSuite::add(const std::string &groupName, const std::string &name,
                   std::function<void(TestSuite &)> testFn) {
    auto test = tests.insert(tests.begin(), std::make_unique<Test>(Test{groupName, name, testFn}));

    auto group = testsByGroup.find(groupName);
    if (group == testsByGroup.end()) {
        testsByGroup.insert(group, {groupName, {*test->get()}});
    } else {
        group->second.push_back(*test->get());
    }

    return 0;
}

std::vector<std::string> TestSuite::files_in(const std::string &path) const {
    auto fullPath = std::filesystem::path(config.resourcesPath) / path;

    std::vector<std::string> paths;

    for (auto it : std::filesystem::directory_iterator(fullPath)) {
        std::string name = it.path().filename().string();
        if (name.find(".") == 0) {
            continue;
        }

        paths.push_back(path / it.path().filename());
    }

    return paths;
}

std::string TestSuite::file_contents(const std::string &path) const {
    auto fullPath = std::filesystem::path(config.resourcesPath) / path;

    std::ifstream file(fullPath);
    std::string contents;

    if (!file) {
        config.out << "Could not open file at path: " << fullPath << std::endl;
        return contents;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    contents = ss.str();
    return contents;
}

std::string TestSuite::basename(const std::string &p) const {
    auto path = std::filesystem::path(p);
    return path.filename();
}

std::string TestSuite::dirname(const std::string &p) const {
    auto path = std::filesystem::path(p);
    path.remove_filename();
    return path;
}
