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
#include "Error.h"

SIF_NAMESPACE_BEGIN

class Reporter {
public:
    virtual void report(const Error &error) = 0;
};

class BasicReporter : public Reporter {
public:
    BasicReporter(const std::string &name, const std::string &source);

    void report(const Error &error) override;

private:
    const std::string &_name;
    const std::string &_source;
};

class CaptureReporter : public Reporter {
public:
    virtual ~CaptureReporter() = default;

    void report(const Error &error) override;

    const std::vector<Error> &errors() const;

private:
    std::vector<Error> _errors;
};

SIF_NAMESPACE_END
