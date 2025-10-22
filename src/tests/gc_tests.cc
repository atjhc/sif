//
//  Copyright (c) 2025 James Callender
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

#include "tests/TestSuite.h"
#include "tests/TrackingObject.h"

#include <sif/compiler/Compiler.h>
#include <sif/compiler/Parser.h>
#include <sif/compiler/Reader.h>
#include <sif/compiler/Reporter.h>
#include <sif/compiler/Scanner.h>
#include <sif/compiler/Signature.h>
#include <sif/runtime/ModuleLoader.h>
#include <sif/runtime/VirtualMachine.h>
#include <sif/runtime/modules/Core.h>
#include <sif/runtime/modules/System.h>
#include <sif/runtime/objects/Dictionary.h>
#include <sif/runtime/objects/List.h>
#include <sif/runtime/objects/Native.h>

#include <random>
#include <sstream>

using namespace sif;

TEST_CASE(GarbageCollector, TracksAllocationDebtOnNewContainers) {
    VirtualMachineConfig config;
    config.initialGarbageCollectionThresholdBytes = 1024;
    config.minimumGarbageCollectionThresholdBytes = 256;
    config.garbageCollectionGrowthFactor = 1.0;

    VirtualMachine vm(config);

    ASSERT_EQ(vm.bytesSinceLastCollection(), 0u);

    auto list = vm.make<List>(std::vector<Value>(16, Value(1)));
    vm.notifyContainerMutation(list.get());
    vm.serviceGarbageCollection();

    ASSERT_GT(vm.currentTrackedBytes(), 0u);

    auto dictionary = vm.make<Dictionary>();
    dictionary->values()[Value(1)] = Value(2);
    vm.notifyContainerMutation(dictionary.get());
    vm.serviceGarbageCollection();

    ASSERT_GTE(vm.currentTrackedBytes(), sizeof(List));
}

TEST_CASE(GarbageCollector, ReportsLowerTrackedBytesAfterReclaim) {
    VirtualMachineConfig config;
    config.initialGarbageCollectionThresholdBytes = 64;
    config.minimumGarbageCollectionThresholdBytes = 32;
    config.garbageCollectionGrowthFactor = 1.0;

    VirtualMachine vm(config);

    {
        auto list = vm.make<List>(std::vector<Value>(16, Value(42)));
        vm.notifyContainerMutation(list.get());
        ASSERT_GT(vm.currentTrackedBytes(), 0u);
    }

    vm.serviceGarbageCollection();

    ASSERT_EQ(vm.currentTrackedBytes(), 0u);
}

TEST_CASE(GarbageCollector, MutationNotificationsIncreaseDebt) {
    VirtualMachineConfig config;
    config.initialGarbageCollectionThresholdBytes = 512;
    config.minimumGarbageCollectionThresholdBytes = 128;
    config.garbageCollectionGrowthFactor = 1.0;

    VirtualMachine vm(config);

    auto list = vm.make<List>();
    auto debtBefore = vm.bytesSinceLastCollection();

    list->values().reserve(64);
    for (int i = 0; i < 64; ++i) {
        list->values().push_back(Value(i));
    }
    auto gcBefore = vm.garbageCollectionCount();
    vm.notifyContainerMutation(list.get());

    auto debtAfterInitialMutation = vm.bytesSinceLastCollection();

    ASSERT_TRUE(debtAfterInitialMutation > debtBefore || vm.garbageCollectionCount() > gcBefore);

    for (int iteration = 0; iteration < 8 && vm.garbageCollectionCount() == gcBefore; ++iteration) {
        list->values().insert(list->values().end(), 32, Value(iteration));
        vm.notifyContainerMutation(list.get());
    }

    vm.serviceGarbageCollection();

    ASSERT_GT(vm.garbageCollectionCount(), gcBefore);
}

static void PopulateCoreSystem(Parser &parser, Core &core, System &system) {
    parser.declare(core.signatures());
    parser.declare(system.signatures());
}

static Compiler MakeCompiler(ModuleLoader &loader, Reporter &reporter) {
    CompilerConfig compilerConfig{loader, reporter, false, true};
    return Compiler(compilerConfig);
}

static void InstallCoreSystem(VirtualMachine &vm, Core &core, System &system) {
    for (const auto &entry : core.values()) {
        vm.addGlobal(entry.first, entry.second);
    }
    for (const auto &entry : system.values()) {
        vm.addGlobal(entry.first, entry.second);
    }
}

TEST_CASE(GarbageCollector, PreservesNativeAllocationsDuringCall) {
    const std::string source = R"(
set rows to transient list
rows
)";

    VirtualMachineConfig vmConfig;
    vmConfig.initialGarbageCollectionThresholdBytes = 0;
    vmConfig.minimumGarbageCollectionThresholdBytes = 0;
    vmConfig.garbageCollectionGrowthFactor = 1.0;

    std::ostringstream out, err;
    std::istringstream in;

    Scanner scanner;
    StringReader reader(source);
    ModuleLoader loader;
    IOReporter reporter(err);
    ParserConfig parserConfig{scanner, reader, loader, reporter};
    Parser parser(parserConfig);

    CoreConfig coreConfig{std::mt19937_64()};
    coreConfig.randomInteger = [&coreConfig](Integer max) { return coreConfig.engine() % max; };
    Core core(coreConfig);
    SystemConfig systemConfig{out, in, err};
    System system(systemConfig);

    PopulateCoreSystem(parser, core, system);
    auto transientListSignature = Signature::Make("transient list");
    ASSERT_TRUE(transientListSignature.has_value());
    parser.declare(transientListSignature.value());

    auto statement = parser.statement();
    ASSERT_FALSE(parser.failed());

    auto compiler = MakeCompiler(loader, reporter);
    auto bytecode = compiler.compile(*statement);
    ASSERT_TRUE(bytecode);

    VirtualMachine vm(vmConfig);
    InstallCoreSystem(vm, core, system);

    auto transientList = MakeStrong<Native>([](const NativeCallContext &context) -> Result<Value, Error> {
        auto list = context.vm.make<List>();
        list->values().push_back(Value(1));
        context.vm.notifyContainerMutation(list.get());
        context.vm.serviceGarbageCollection();
        return Value(list);
    });
    vm.addGlobal("transient list", transientList);

    auto gcCountBefore = vm.garbageCollectionCount();

    auto execResult = vm.execute(bytecode);
    ASSERT_TRUE(execResult.has_value());

    ASSERT_GT(vm.garbageCollectionCount(), gcCountBefore);

    auto rowsValue = execResult.value();
    auto rows = rowsValue.as<List>();
    ASSERT_TRUE(rows);
    ASSERT_EQ(rows->values().size(), 1u);
    ASSERT_TRUE(rows->values()[0].isInteger());
    ASSERT_EQ(rows->values()[0].asInteger(), 1);
}

TEST_CASE(GarbageCollector, ReleasesTransientAllocationsWithoutRoots) {
    const std::string source = R"(
transient scratch
collect garbage
)";

    VirtualMachineConfig vmConfig;
    vmConfig.initialGarbageCollectionThresholdBytes = 0;
    vmConfig.minimumGarbageCollectionThresholdBytes = 0;
    vmConfig.garbageCollectionGrowthFactor = 1.0;

    std::ostringstream out, err;
    std::istringstream in;

    Scanner scanner;
    StringReader reader(source);
    ModuleLoader loader;
    IOReporter reporter(err);
    ParserConfig parserConfig{scanner, reader, loader, reporter};
    Parser parser(parserConfig);

    CoreConfig coreConfig{std::mt19937_64()};
    coreConfig.randomInteger = [&coreConfig](Integer max) { return coreConfig.engine() % max; };
    Core core(coreConfig);
    SystemConfig systemConfig{out, in, err};
    System system(systemConfig);

    PopulateCoreSystem(parser, core, system);
    auto transientScratchSignature = Signature::Make("transient scratch");
    ASSERT_TRUE(transientScratchSignature.has_value());
    parser.declare(transientScratchSignature.value());
    auto collectGcSignature = Signature::Make("collect garbage");
    ASSERT_TRUE(collectGcSignature.has_value());
    parser.declare(collectGcSignature.value());

    auto statement = parser.statement();
    ASSERT_FALSE(parser.failed());

    auto compiler = MakeCompiler(loader, reporter);
    auto bytecode = compiler.compile(*statement);
    ASSERT_TRUE(bytecode);

    VirtualMachine vm(vmConfig);
    InstallCoreSystem(vm, core, system);

    TrackingObject::count = 0;
    auto transientScratch = MakeStrong<Native>([](const NativeCallContext &context) -> Result<Value, Error> {
        auto list = context.vm.make<List>();
        list->values().push_back(Value(context.vm.make<TrackingObject>()));
        context.vm.notifyContainerMutation(list.get());
        context.vm.serviceGarbageCollection();
        return Value();
    });
    vm.addGlobal("transient scratch", transientScratch);

    vm.addGlobal("collect garbage", MakeStrong<Native>([](const NativeCallContext &context) -> Result<Value, Error> {
        context.vm.serviceGarbageCollection();
        return Value();
    }));

    auto execResult = vm.execute(bytecode);
    ASSERT_TRUE(execResult.has_value());

    vm.serviceGarbageCollection();
    ASSERT_EQ(TrackingObject::count, 0);
}
