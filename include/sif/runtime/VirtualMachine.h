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

#include <atomic>
#include <stack>
#include <type_traits>
#include <vector>

SIF_NAMESPACE_BEGIN

class List;
class Dictionary;

struct VirtualMachineConfig {
#if defined(DEBUG)
    // When true the VM dumps opcode-level tracing to stdout for debugging.
    bool enableTracing = false;
#endif
    // Starting threshold for bytes allocated before the GC is triggered automatically.
    size_t initialGarbageCollectionThresholdBytes = 64 * 1024;
    // Lower bound for the GC trigger threshold so it never shrinks too aggressively.
    size_t minimumGarbageCollectionThresholdBytes = 16 * 1024;
    // Multiplier applied to the next threshold after each successful collection.
    double garbageCollectionGrowthFactor = 1.5;
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
    ~VirtualMachine();

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

    void notifyContainerMutation(List *list);
    void notifyContainerMutation(Dictionary *dictionary);

    void serviceGarbageCollection();

    size_t bytesSinceLastCollection() const { return _bytesSinceLastGc; }
    size_t currentTrackedBytes() const { return _liveContainerBytes; }
    size_t garbageCollectionCount() const { return _garbageCollectionCount; }

    template <class T, class... Args> std::shared_ptr<T> make(Args &&...args) {
        if constexpr (IsTrackedContainer<T>) {
            auto obj = std::shared_ptr<T>(new T(std::forward<Args>(args)...), [this](T *p) {
                deregisterContainer(p);
                delete p;
            });
            if (_inNativeCall) {
                _transientRoots.emplace_back(obj);
            }
            trackContainer(obj);
            return obj;
        } else {
            return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
        }
    }

    void collectGarbage();

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif
    VirtualMachineConfig config;
#ifdef __clang__
#pragma GCC diagnostic pop
#endif

  private:
    Optional<Error> call(Value, int, std::vector<SourceRange>);
    Optional<Error> range(Value, Value, bool);

    CallFrame &frame();

    void trackContainer(const Strong<Object> &container);
    std::vector<Strong<Object>> gatherRootObjects() const;

    void refreshContainerMetrics(bool accumulateDebt);
    void maybeTriggerGarbageCollection();
    void runPendingGarbageCollection();
    void cleanupExpiredContainers();
    void deregisterContainer(Object *object);
    void accountForContainer(Object *object, size_t newSize, bool accumulateDebt);
    size_t estimateContainerSize(const Object *object) const;

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
    Mapping<Object *, size_t> _containerSizes;
    size_t _bytesSinceLastGc = 0;
    size_t _nextGcThreshold = 0;
    size_t _liveContainerBytes = 0;
    size_t _garbageCollectionCount = 0;
    bool _gcInProgress = false;
    bool _gcPending = false;
    bool _inNativeCall = false;
    std::vector<Weak<Object>> _transientRoots;
};

SIF_NAMESPACE_END
