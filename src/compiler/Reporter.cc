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

#include "Reporter.h"
#include "utilities/chunk.h"

#include <iostream>

SIF_NAMESPACE_BEGIN

BasicReporter::BasicReporter(const std::string &name, const std::string &source)
    : _name(name), _source(source) {}

void BasicReporter::report(const Error &error) {
    std::cerr << _name << ":" << error.location() << ": Error: " << error.what() << std::endl;
    if (error.location().position > 0) {
        std::cerr << index_chunk(chunk::line, error.location().lineNumber - 1, _source).get()
                  << std::endl;
        std::cerr << std::string(error.location().position - 1, ' ') << "^" << std::endl;
    }
}

IOReporter::IOReporter(std::ostream &err) : _err(err) {}

void IOReporter::report(const Error &error) { _err << error.what() << std::endl; }

void CaptureReporter::report(const Error &error) { _errors.push_back(error); }

const std::vector<Error> &CaptureReporter::errors() const { return _errors; }

SIF_NAMESPACE_END
