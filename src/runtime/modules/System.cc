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
#include "runtime/objects/String.h"

#include <filesystem>
#include <fstream>
#include <sys/utsname.h>

SIF_NAMESPACE_BEGIN

using ModuleMap = Mapping<Signature, Strong<Native>, Signature::Hash>;
static Signature S(const char *signature) { return Signature::Make(signature).value(); }

void System::_files(ModuleMap &natives) {
    natives[S("(the) contents of file {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto path = values[0].as<String>();
            if (!path) {
                return Fail(Error(location, "expected a string"));
            }
            std::ifstream file(path->string());
            if (!file.is_open()) {
                return Fail(Error(location, "unable to open file"));
            }
            std::ostringstream sstr;
            sstr << file.rdbuf();
            return Value(sstr.str());
        });
    natives[S("(the) contents of directory {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto path = values[0].as<String>();
            if (!path) {
                return Fail(Error(location, "expected a string"));
            }
            std::error_code error;
            auto it = std::filesystem::directory_iterator(path->string(), error);
            if (error) {
                return Fail(Error(location, Value(error.message())));
            }
            Strong<List> results = MakeStrong<List>();
            for (auto it : std::filesystem::directory_iterator(path->string())) {
                auto itemPath = it.path();
                results->values().push_back(itemPath.string());
            }
            return results;
        });
    natives[S("remove file {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto pathString = values[0].as<String>();
            if (!pathString) {
                return Fail(Error(location, "expected a string"));
            }
            auto path = std::filesystem::path(pathString->string());

            std::error_code error;
            std::filesystem::remove(path, error);
            if (error) {
                return Fail(Error(location, Value(error.message())));
            }

            return Value();
        });
    natives[S("remove directory {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto pathValue = values[0].as<String>();
            if (!pathValue) {
                return Fail(Error(location, "expected a string"));
            }
            auto path = std::filesystem::path(pathValue->string());

            std::error_code error;
            std::filesystem::remove_all(path, error);
            if (error) {
                return Fail(Error(location, Value(error.message())));
            }

            return Value();
        });
    natives[S("move file/directory {} to {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto fromValue = values[0].as<String>();
            if (!fromValue) {
                return Fail(Error(location, "expected a string"));
            }
            auto toValue = values[1].as<String>();
            if (!toValue) {
                return Fail(Error(location, "expected a string"));
            }
            auto from = std::filesystem::path(fromValue->string());
            auto to = std::filesystem::path(toValue->string());

            std::error_code error;
            std::filesystem::rename(from, to, error);
            if (error) {
                return Fail(Error(location, Value(error.message())));
            }

            return Value();
        });
    natives[S("copy file/directory {} to {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto fromValue = values[0].as<String>();
            if (!fromValue) {
                return Fail(Error(location, "expected a string"));
            }
            auto toValue = values[0].as<String>();
            if (!toValue) {
                return Fail(Error(location, "expected a string"));
            }
            auto from = std::filesystem::path(fromValue->string());
            auto to = std::filesystem::path(toValue->string());

            std::error_code error;
            std::filesystem::copy(from, to, error);
            if (error) {
                return Fail(Error(location, Value(error.message())));
            }

            return Value();
        });
}

System::System() {
    _natives[S("the arguments")] = MakeStrong<Native>(
        [this](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return MakeStrong<List>(_arguments.begin(), _arguments.end());
        });
    _natives[S("the environment")] = MakeStrong<Native>(
        [this](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto dictionary = MakeStrong<Dictionary>();
            for (auto pair : _environment) {
                dictionary->values()[Value(pair.first)] = Value(pair.second);
            }
            return dictionary;
        });
    _natives[S("the clock")] =
        MakeStrong<Native>([](CallFrame &frame, SourceLocation location,
                              Value *values) -> Result<Value, Error> { return Integer(clock()); });
    _natives[S("the system name")] =
        MakeStrong<Native>([this](CallFrame &frame, SourceLocation location,
                                  Value *values) -> Result<Value, Error> { return _systemName; });
    _natives[S("the system version")] = MakeStrong<Native>(
        [this](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return _systemVersion;
        });
    _files(_natives);
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

Mapping<std::string, Value> System::values() const {
    Mapping<std::string, Value> values;
    for (const auto &native : _natives) {
        values.insert({native.first.name(), native.second});
    }
    return values;
}

SIF_NAMESPACE_END
