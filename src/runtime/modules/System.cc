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

#include "Utilities.h"
#include "sif/Error.h"

#include "extern/utf8.h"
#include "sif/runtime/VirtualMachine.h"
#include "sif/runtime/modules/System.h"
#include "sif/runtime/objects/Dictionary.h"
#include "sif/runtime/objects/List.h"
#include "sif/runtime/objects/String.h"

#include <filesystem>
#include <fstream>
#include <sys/utsname.h>

SIF_NAMESPACE_BEGIN

#define N(X) MakeStrong<Native>(X)

using ModuleMap = Mapping<Signature, Strong<Native>, Signature::Hash>;
static Signature S(const char *signature) { return Signature::Make(signature).value(); }

static auto _write_T(std::ostream &out)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&out](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                out << Join(list->values(), " ");
            } else {
                out << values[0];
            }
            return Value();
        };
}

static auto _write_error_T(std::ostream &err)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&err](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                err << Join(list->values(), " ");
            } else {
                err << values[0];
            }
            return Value();
        };
}

static auto _print_T(std::ostream &out)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&out](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                out << Join(list->values(), " ");
            } else {
                out << values[0];
            }
            out << std::endl;
            return Value();
        };
}

static auto _print_error_T(std::ostream &err)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&err](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                for (const auto &item : list->values()) {
                    err << item;
                }
            } else {
                err << values[0];
            }
            err << std::endl;
            return Value();
        };
}

static auto _read_a_word(std::istream &in)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&in](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::string input;
            in >> input;
            return input;
        };
}

static auto _read_a_line(std::istream &in)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&in](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::string input;
            std::getline(in, input);
            return input;
        };
}

static auto _read_a_character(std::istream &in)
    -> std::function<Result<Value, Error>(VirtualMachine &, SourceLocation, Value *)> {
    return
        [&in](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::istreambuf_iterator<char> it(in.rdbuf());
            std::istreambuf_iterator<char> eos;
            std::string result;
            try {
                char32_t input = utf8::next(it, eos);
                result = utf8::utf32to8(std::u32string_view(&input, 1));
            } catch (const utf8::exception &exception) {
                return Fail(Error(location, exception.what()));
            }
            return result;
        };
}

static auto _the_contents_of_file_T(VirtualMachine &vm, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _the_contents_of_directory_T(VirtualMachine &vm, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto path = values[0].as<String>();
    if (!path) {
        return Fail(Error(location, "expected a string"));
    }
    std::error_code error;
    auto it = std::filesystem::directory_iterator(path->string(), error);
    if (error) {
        return Fail(Error(location, Value(error.message())));
    }
    Strong<List> results = vm.make<List>();
    for (auto it : std::filesystem::directory_iterator(path->string())) {
        auto itemPath = it.path();
        results->values().push_back(itemPath.string());
    }
    return results;
}

static auto _remove_file_T(VirtualMachine &vm, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _remove_directory_T(VirtualMachine &vm, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _move_T_to_T(VirtualMachine &vm, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _copy_T_to_T(VirtualMachine &vm, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static void _io(ModuleMap &natives, std::ostream &out, std::istream &in, std::ostream &err) {
    natives[S("write {}")] = N(_write_T(out));
    natives[S("write error {}")] = N(_write_error_T(err));
    natives[S("print {}")] = N(_print_T(out));
    natives[S("print error {}")] = N(_print_error_T(err));
    natives[S("read (a) word")] = N(_read_a_word(in));
    natives[S("read (a) line")] = N(_read_a_line(in));
    natives[S("read (a) character")] = N(_read_a_character(in));
}

static void _files(ModuleMap &natives) {
    natives[S("(the) contents of file {}")] = N(_the_contents_of_file_T);
    natives[S("(the) contents of directory {}")] = N(_the_contents_of_directory_T);
    natives[S("remove file {}")] = N(_remove_file_T);
    natives[S("remove directory {}")] = N(_remove_directory_T);
    natives[S("move file/directory {} to {}")] = N(_move_T_to_T);
    natives[S("copy file/directory {} to {}")] = N(_copy_T_to_T);
}

System::System(const SystemConfig &config) {
    _natives[S("the arguments")] = MakeStrong<Native>(
        [this](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            return vm.make<List>(_arguments.begin(), _arguments.end());
        });
    _natives[S("the environment")] = MakeStrong<Native>(
        [this](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto dictionary = vm.make<Dictionary>();
            for (auto pair : _environment) {
                dictionary->values()[Value(pair.first)] = Value(pair.second);
            }
            return dictionary;
        });
    _natives[S("the clock")] =
        MakeStrong<Native>([](VirtualMachine &vm, SourceLocation location,
                              Value *values) -> Result<Value, Error> { return Integer(clock()); });
    _natives[S("the system name")] =
        MakeStrong<Native>([this](VirtualMachine &vm, SourceLocation location,
                                  Value *values) -> Result<Value, Error> { return _systemName; });
    _natives[S("the system version")] = MakeStrong<Native>(
        [this](VirtualMachine &vm, SourceLocation location, Value *values) -> Result<Value, Error> {
            return _systemVersion;
        });
    _io(_natives, config.out, config.in, config.err);
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
