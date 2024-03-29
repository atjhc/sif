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

SIF_NAMESPACE_BEGIN

struct ModuleLoaderProxy : public ModuleProvider {
    ModuleLoaderProxy(ModuleLoader &loader);
    virtual ~ModuleLoaderProxy() = default;

    Strong<Module> module(const std::string &source, std::vector<Error> &errors,
                          bool &circular) override;

    ModuleLoader &loader;
};

ModuleLoaderProxy::ModuleLoaderProxy(ModuleLoader &loader) : loader(loader) {}

Strong<Module> ModuleLoaderProxy::module(const std::string &source, std::vector<Error> &errors,
                                         bool &circular) {
    return loader.module(source, errors, circular);
}

ModuleLoader::ModuleLoader() {}

Strong<Module> ModuleLoader::module(const std::string &source, std::vector<Error> &errors,
                                    bool &circular) {
    // Check if the module is already in the process of loading,
    // which implies a circular dependency.
    if (_loading.find(source) != _loading.end()) {
        circular = true;
        return nullptr;
    }

    // Check if the module has already been loaded.
    auto it = _modules.find(source);
    if (it != _modules.end()) {
        return it->second;
    }

    _loading.insert(source);

    auto scanner = Scanner();
    auto reader = FileReader(source);

    ParserConfig parserConfig{scanner, reader, *this};
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
        errors.insert(errors.end(), parser.errors().begin(), parser.errors().end());
        return nullptr;
    }

    // Compile the bytecode for the new module.
    CompilerConfig compilerConfig{*this};
    Compiler compiler(compilerConfig);
    auto bytecode = compiler.compile(*statement);
    if (!bytecode) {
        errors.insert(errors.end(), compiler.errors().begin(), compiler.errors().end());
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
        errors.push_back(result.error());
        return nullptr;
    }

    _loading.erase(source);
    auto userModule = MakeStrong<UserModule>(source, parser.declarations(), vm.exports());
    _modules[source] = userModule;
    return userModule;
}

SIF_NAMESPACE_END
