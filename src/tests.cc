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

// #include "tests/common.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <dirent.h>

struct TestSuite;

#define TEST(x) {#x, x}
struct Test {
    std::string name;
    std::function<void(TestSuite&)> test;
};

#define assert_true(c) \
    _assert_true(c, #c, __FILE__, __LINE__)

struct TestSuite {
    std::string resourcesPath;
    
    TestSuite(std::string _resourcesPath) : resourcesPath(_resourcesPath) {}

    void add(const Test &test) {
        tests.push_back(test);
    }

    int run() {
        std::cout << "Running " << tests.size() << " tests." << std::endl;
        for (auto &test : tests) {
            std::cout << "Running " << test.name << std::endl;
            test.test(*this);
            std::cout << "Finished " << test.name << std::endl;
        }
        std::cout << "Ran " << failure_count + success_count << " tests with " 
            << success_count << " successes and " << failure_count << " failures." << std::endl;

        return failure_count;
    }

    std::vector<std::string> files_in(const std::string &path) const {
        auto fullPath = resourcesPath + '/' + path;
        DIR *directory = opendir(fullPath.c_str());
        std::vector<std::string> paths;

        if (!directory) {
            std::cerr << "Could not open directory at path: " << fullPath << std::endl;
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

    std::string file_contents(const std::string &path) const {
        auto fullPath = resourcesPath + '/' + path;
        std::ifstream file(fullPath);
        std::string contents;
        
        if (!file) {
            std::cerr << "Could not open file at path: " << fullPath << std::endl;
            return contents;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        contents = ss.str();
        return contents;
    }

    void _assert_true(bool condition, std::string msg = "", std::string file = __FILE__, int line = __LINE__) {
        if (condition) {
            success_count++;
        } else {
            std::cout << "Test \"" << msg << "\" failed. (" << file << ":" << line << ")" << std::endl;
            failure_count++;
        }
    }

private:
    std::vector<Test> tests;
    int success_count;
    int failure_count;
};

#include "tests/parse_tests.cc"

int main(int argc, char *argv[]) {
    auto tests = TestSuite(argv[1]);
    tests.add(TEST(parse_tests));
    return tests.run();
}
