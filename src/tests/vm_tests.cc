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

#include "runtime/VirtualMachine.h"
#include "compiler/Bytecode.h"
#include "tests/TestSuite.h"

#include <filesystem>
#include <iostream>
#include <sstream>

using namespace chatter;

TEST_CASE(VMTests, All) {
    auto bytecode = MakeStrong<Bytecode>();
    auto index = bytecode->add(Value(10.0));
    bytecode->add(Opcode::Constant, index);
    index = bytecode->add(Value(5.0));
    bytecode->add(Opcode::Constant, index);
    bytecode->add(Opcode::Add);
    bytecode->add(Opcode::Return);

    auto vm = VirtualMachine(bytecode);
    ASSERT_EQ(vm.execute().value().asFloat(), 15.0);
}
