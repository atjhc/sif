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
#include "compiler/Bytecode.h"
#include "runtime/Value.h"
#include "Error.h"

#include <stack>
#include <vector>

SIF_NAMESPACE_BEGIN

struct VirtualMachineConfig {
#if defined(DEBUG)
    bool enableTracing = false;
#endif
};

struct CallFrame {
    Strong<Bytecode> bytecode;
    Bytecode::Iterator ip;
    std::vector<size_t> captures;
    size_t sp;
    std::vector<Bytecode::Iterator> jumps;
    std::vector<size_t> sps;
    Value error;

    CallFrame(Strong<Bytecode> bytecode, const std::vector<size_t> &captures, size_t sp)
        : bytecode(bytecode), ip(bytecode->code().begin()), captures(captures), sp(sp) {}
};

class VirtualMachine {
  public:
    VirtualMachine(const VirtualMachineConfig &config = VirtualMachineConfig());

    Optional<Value> execute(const Strong<Bytecode> &bytecode);
    void add(const std::string &name, const Value nativeFunction);

    Optional<RuntimeError> error() const;

  private:
    Optional<RuntimeError> call(Value, int count);
    Optional<RuntimeError> range(Value, Value, bool);

    CallFrame &frame();

#if defined(DEBUG)
    friend std::ostream &operator<<(std::ostream &out, const CallFrame &f);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
    VirtualMachineConfig _config;
#pragma GCC diagnostic pop

    Optional<RuntimeError> _error;
    std::vector<Value> _stack;
    std::vector<CallFrame> _frames;
    Mapping<std::string, Value> _variables;
    Value _it;
};

SIF_NAMESPACE_END
