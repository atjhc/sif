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
#include <libgen.h>

int usage(int argc, char *argv[]) {
    std::cout << "Usage: " << basename(argv[0]) << " [options...] [file]" << std::endl
              << " -t, --test"
              << "\t Run a specific test, requires -g" << std::endl
              << " -g, --group"
              << "\t Specify a group to test" << std::endl
              << " -h, --help"
              << "\t Print out this help and exit" << std::endl;
    return -1;
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"group", required_argument, NULL, 'g'},
        {"test", required_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

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

    if (testName.empty() && groupName.empty()) {
        return chatter::MainTestSuite().runAll();
    } else if (testName.empty() && !groupName.empty()) {
        return chatter::MainTestSuite().runGroup(groupName);
    } else {
        return chatter::MainTestSuite().runTest(groupName, testName);
    }
}
