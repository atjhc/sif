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

#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/param.h>
#include <fstream>
#include <sstream>

CH_NAMESPACE_BEGIN

TestSuite &MainTestSuite() {
    static TestSuite mainTestSuite;
    return mainTestSuite;
}

int RunAllTests() {
    return MainTestSuite().runAll();
}

int TestSuite::runAll() {
    config.out << "Running " << tests.size() << " test cases" << std::endl;
    for (auto &group : testsByGroup) {
        _run(group.first, group.second);
    }

    return _summarize();
}

int TestSuite::runGroup(const std::string &groupName) {
    auto g = testsByGroup.find(groupName);
    if (g == testsByGroup.end()) {
        config.out << "Could not find group named " << groupName << std::endl;
        return -1;
    }

    config.out << "Running " << g->second.size() << " test cases" << std::endl;
    _run(g->first, g->second);

    return _summarize();
}

int TestSuite::runTest(const std::string &groupName, const std::string &testName) {
    auto g = testsByGroup.find(groupName);
    if (g == testsByGroup.end()) {
        config.out << "Could not find group named " << groupName << std::endl;
        return -1;
    }

    for (auto &test : g->second) {
        if (test.get().name == testName) {
            _run(test.get());
            break;
        }
    }

    return _summarize();
}

int TestSuite::_summarize() {
    config.out << "Ran " << failure_count + success_count << " tests with " << success_count
               << " successes and " << failure_count << " failures." << std::endl;
    return failure_count;
}

void TestSuite::_run(const std::string &name, const std::vector<Ref<Test>> &tests) {
    config.out << "Running test group " << name << std::endl;
    for (auto &test : tests) {
        _run(test.get());
    }
    config.out << "Finished test group " << name << std::endl;
}

void TestSuite::_run(const Test &test) {
    config.out << "Running test " << test.name << std::endl;
    test.test(*this);
    config.out << "Finished test " << test.name << std::endl;
}

void TestSuite::_assert(bool condition, const char *test, const char *file, int line) {
    if (condition) {
        success_count++;
    } else {
        config.out << "Test \"" << test << "\" failed. (" << file << ":" << line << ")"
                   << std::endl;
        failure_count++;
    }
}

int TestSuite::add(const std::string &groupName, const std::string &name, std::function<void(TestSuite &)> testFn) { 
    auto test = tests.insert(tests.begin(), MakeOwned<Test>(Test{groupName, name, testFn}));
    
    auto group = testsByGroup.find(groupName);
    if (group == testsByGroup.end()) {
        testsByGroup.insert(group, {groupName, {*test->get()}});
    } else {
        group->second.push_back(*test->get());
    }

    return 0;
}

std::vector<std::string> TestSuite::files_in(const std::string &path) const {
    auto fullPath = config.resourcesPath + '/' + path;
    DIR *directory = opendir(fullPath.c_str());
    std::vector<std::string> paths;

    if (!directory) {
        config.out << "Could not open directory at path: " << fullPath << std::endl;
        return paths;
    }

    while (struct dirent *entry = readdir(directory)) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        paths.push_back(path + '/' + name);
    }

    return paths;
}

std::string TestSuite::file_contents(const std::string &path) const {
    auto fullPath = config.resourcesPath + '/' + path;
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

std::string TestSuite::basename(const std::string &path) const {
    char buffer[MAXPATHLEN];
    return std::string(basename_r(path.c_str(), buffer));
}

std::string TestSuite::dirname(const std::string &path) const {
    char buffer[MAXPATHLEN];
    return std::string(dirname_r(path.c_str(), buffer));
}

CH_NAMESPACE_END
