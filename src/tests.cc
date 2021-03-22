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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>

#include "parser/Parser.h"

using namespace chatter;
using namespace chatter::ast;

static int parseTest(const std::string &path) {
    assert(!path.empty());

    std::string source;
    std::ifstream file(path);

    assert(file);

    std::ostringstream ss;
    ss << file.rdbuf();
    source = ss.str();

    ParserConfig config;
    config.fileName = path;

    return (Parser().parse(config, source) == nullptr);
}

static int runTests(const std::string &path) {
    DIR *testsDirectory = opendir(path.c_str());
    if (!testsDirectory) {
        std::cerr << "Could not open directory at path: " << path << std::endl;
    }
    
    std::vector<std::string> fileNames;
    while (struct dirent *entry = readdir(testsDirectory)) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        fileNames.push_back(name);
    }
    
    int failureCount = 0;
    int passCount = 0;
    std::cout << "Running " << fileNames.size() << " parse tests." << std::endl;
    for (auto fileName : fileNames) {
        int result = parseTest(path + "/" + fileName);
        if (result) {
            failureCount++;
        } else {
            passCount++;
        }
    }

    std::cout << "Ran " << fileNames.size() << " parse tests with " 
        << failureCount << " failures and " << passCount << " successes." << std::endl;

    return 0;
}

int main(int argc, char *argv[]) {
    return runTests(argv[1]);
}
