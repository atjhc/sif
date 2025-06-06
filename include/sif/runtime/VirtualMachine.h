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

#include <sif/Common.h>
#include <sif/Error.h>
#include <sif/compiler/Bytecode.h>
#include <sif/runtime/Value.h>

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
    Value it;

    CallFrame(Strong<Bytecode> bytecode, const std::vector<size_t> &captures, size_t sp)
        : bytecode(bytecode), ip(bytecode->code().begin()), captures(captures), sp(sp) {}
};

class VirtualMachine {
  public:
    VirtualMachine(const VirtualMachineConfig &config = VirtualMachineConfig());

    Result<Value, Error> execute(const Strong<Bytecode> &bytecode);
    void requestHalt();

    void addGlobal(const std::string &name, const Value value);
    void addGlobals(const Mapping<std::string, Value> &globals);

    const Mapping<std::string, Value> globals() const;
    const Mapping<std::string, Value> exports() const;

    const Value &error() const { return _frames.back().error; }
    Value &error() { return _frames.back().error; }

    const Value &it() const { return _it; }
    Value &it() { return _it; }

    template <class T, class... Args> std::shared_ptr<T> make(Args &&...args) {
        auto obj = std::shared_ptr<T>(new T(std::forward<Args>(args)...), [&](auto p) {
            _trackedContainers.erase(p);
            delete p;
        });
        trackObject(obj);
        return obj;
    }

    void collectGarbage();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
    VirtualMachineConfig config;
#pragma GCC diagnostic pop

  private:
    Optional<Error> call(Value, int, std::vector<SourceRange>);
    Optional<Error> range(Value, Value, bool);

    CallFrame &frame();

    void trackObject(Strong<Object> object);
    std::vector<Strong<Object>> gatherRootObjects() const;

#if defined(DEBUG)
    friend std::ostream &operator<<(std::ostream &out, const CallFrame &f);
#endif

    std::atomic<bool> _haltRequested{false};
    std::vector<Value> _stack;
    std::vector<CallFrame> _frames;
    Mapping<std::string, Value> _globals;
    Mapping<std::string, Value> _exports;
    Value _it;

    // Garbage collection state
    Mapping<Object *, Weak<Object>> _trackedContainers;
};

SIF_NAMESPACE_END
