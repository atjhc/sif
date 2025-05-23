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

#include "tests/TestSuite.h"
#include "utilities/chunk.h"

#include "sif/sif.h"

using namespace sif;

class TrackingObject : public Object {
  public:
    TrackingObject() {
        count++;
    }
    ~TrackingObject() {
        count--;
    }

    std::string typeName() const override { return "TrackingObject"; }
    std::string description() const override { return "TrackingObject"; }

    static int count;
};

int TrackingObject::count = 0;

TEST_CASE(GCTests, CycleCollection) {
    std::string source = "set a to [tracking object]\n"
                         "set b to []\n"
                         "insert a at the end of b\n"
                         "insert b at the end of a\n";

    auto scanner = Scanner();
    auto reader = StringReader(source);
    auto loader = ModuleLoader();
    auto reporter = CaptureReporter();

    ParserConfig config{scanner, reader, loader, reporter};
    Parser parser(config);

    CoreConfig coreConfig{std::mt19937_64()};
    coreConfig.randomInteger = [&coreConfig](Integer max) {
        return coreConfig.engine() % max;
    };

    Core core;
    System system;

    parser.declare(core.signatures());
    parser.declare(system.signatures());
    parser.declare(Signature::Make("tracking object").value());

    auto statement = parser.statement();
    ASSERT_FALSE(parser.failed());
    Compiler compiler(CompilerConfig{loader, reporter, false});
    auto bytecode = compiler.compile(*statement);
    ASSERT_NOT_NULL(bytecode);

    {
        VirtualMachine vm;
        vm.addGlobals(core.values());
        vm.addGlobals(system.values());

        vm.addGlobal("tracking object", MakeStrong<Native>([](VirtualMachine &vm, SourceLocation location,
                                                Value *values) -> Result<Value, Error> {
                        return MakeStrong<TrackingObject>();
                    }));

        auto result = vm.execute(bytecode);
        ASSERT_TRUE(result);
    }
    ASSERT_EQ(TrackingObject::count, 0);
}