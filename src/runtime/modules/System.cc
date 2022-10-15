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

#include "Common.h"
#include "Error.h"
#include "Utilities.h"

#include "runtime/VirtualMachine.h"
#include "runtime/modules/System.h"
#include "runtime/objects/Dictionary.h"
#include "runtime/objects/List.h"

#include <sys/utsname.h>

SIF_NAMESPACE_BEGIN

static Signature S(const char *signature) { return Signature::Make(signature).value(); }

System::System() {
    _natives[S("the arguments")] = MakeStrong<Native>(
        [this](CallFrame &frame, Location location, Value *values) -> Result<Value, RuntimeError> {
            return MakeStrong<List>(_arguments.begin(), _arguments.end());
        });
    _natives[S("the environment")] = MakeStrong<Native>(
        [this](CallFrame &frame, Location location, Value *values) -> Result<Value, RuntimeError> {
            auto dictionary = MakeStrong<Dictionary>();
            for (auto pair : _environment) {
                dictionary->values()[Value(pair.first)] = Value(pair.second);
            }
            return dictionary;
        });
    _natives[S("the clock")] = MakeStrong<Native>(
        [](CallFrame &frame, Location location, Value *values) -> Result<Value, RuntimeError> {
            return Integer(clock());
        });
    _natives[S("the system name")] = MakeStrong<Native>(
        [this](CallFrame &frame, Location location, Value *values) -> Result<Value, RuntimeError> {
            return _systemName;
        });
    _natives[S("the system version")] = MakeStrong<Native>(
        [this](CallFrame &frame, Location location, Value *values) -> Result<Value, RuntimeError> {
            return _systemVersion;
        });
}

void System::setArguments(char **argv) {
    _arguments.clear();
    while (*argv) {
        _arguments.push_back(*argv);
        argv++;
    }
}

void System::setEnvironment(char **envp) {
    _environment.clear();
    while (*envp) {
        size_t offset = 0;
        while ((*envp)[offset] != '=')
            offset++;
        _environment[std::string(*envp, offset)] = std::string(*envp + offset + 1);
        envp++;
    }
}

void System::setSystemName(const std::string &systemName) { _systemName = systemName; }

void System::setSystemVersion(const std::string &systemVersion) { _systemVersion = systemVersion; }

std::vector<Signature> System::signatures() const {
    std::vector<Signature> signatures;
    for (const auto &pair : _natives) {
        signatures.push_back(pair.first);
    }
    return signatures;
}

Mapping<std::string, Strong<Native>> System::functions() const {
    Mapping<std::string, Strong<Native>> natives;
    for (const auto &native : _natives) {
        natives.insert({native.first.name(), native.second});
    }
    return natives;
}

SIF_NAMESPACE_END
