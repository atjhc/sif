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

#include "runtime/ModuleLoader.h"

#include "runtime/modules/Core.h"
#include "runtime/modules/System.h"

#include "compiler/Reporter.h"

SIF_NAMESPACE_BEGIN

Result<Strong<Module>, Error> ModuleLoader::module(const std::string &name) {

    // Check if the module is already in the process of loading,
    // which implies a circular dependency.
    if (_loading.find(name) != _loading.end()) {
        return Fail(Error("Circular module import"));
    }

    // Check if the module has already been loaded.
    auto it = _modules.find(name);
    if (it != _modules.end()) {
        return it->second;
    }
    _loading.insert(name);

    std::filesystem::path modulePath;
    for (auto &&location : config.searchPaths) {
        auto path = location / name;
        if (std::filesystem::exists(path)) {
            modulePath = path;
            break;
        }
        if (path.extension().empty()) {
            path += ".sif";
            if (std::filesystem::exists(path)) {
                modulePath = path;
                break;
            }
        }
    }

    if (modulePath.empty()) {
        return Fail(Error(Concat("module ", Quoted(name), " not found")));
    }

    auto scanner = Scanner();
    auto reader = FileReader(modulePath.string());
    auto reporter = BasicReporter(name, reader.contents());
    ParserConfig parserConfig{scanner, reader, *this, reporter};
    Parser parser(parserConfig);

    Core core;
    System system;
    std::vector<Ref<Module>> builtins = {core, system};

    // Import all signatures from builtin modules.
    for (auto ref : builtins) {
        for (const auto &signature : ref.get().signatures()) {
            parser.declare(signature);
        }
    }

    // Parse the new module.
    auto statement = parser.statement();
    if (!statement) {
        return nullptr;
    }

    // Compile the bytecode for the new module.
    CompilerConfig compilerConfig{*this, reporter};
    Compiler compiler(compilerConfig);
    auto bytecode = compiler.compile(*statement);
    if (!bytecode) {
        return nullptr;
    }

    // Insert all symbols linked from builtin modules to make sure they are available at runtime.
    VirtualMachine vm;
    for (auto ref : builtins) {
        for (const auto &pair : ref.get().values()) {
            if (compiler.globals().find(pair.first) != compiler.globals().end()) {
                vm.addGlobal(pair.first, pair.second);
            }
        }
    }

    // Execute the new module to evaluate the exported values.
    auto result = vm.execute(bytecode);
    if (!result) {
        return Fail(result.error());
    }

    _loading.erase(name);
    auto module = MakeStrong<UserModule>(name, parser.declarations(), vm.exports());
    _modules[name] = module;
    return module;
}

SIF_NAMESPACE_END
