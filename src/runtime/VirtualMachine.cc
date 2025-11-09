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

#include "sif/runtime/VirtualMachine.h"
#include "sif/runtime/objects/Dictionary.h"
#include "sif/runtime/objects/Function.h"
#include "sif/runtime/objects/List.h"
#include "sif/runtime/objects/Native.h"
#include "sif/runtime/objects/Range.h"
#include "sif/runtime/objects/String.h"
#include "sif/runtime/protocols/Enumerable.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stack>

SIF_NAMESPACE_BEGIN

VirtualMachine::VirtualMachine(const VirtualMachineConfig &config) : config(config) {
    _nextGcThreshold = std::max(config.initialGarbageCollectionThresholdBytes,
                                config.minimumGarbageCollectionThresholdBytes);
}

VirtualMachine::~VirtualMachine() {
    _stack.clear();
    _frames.clear();
    _globals.clear();
    _exports.clear();
    _it = Value();

    _gcPending = true;
    runPendingGarbageCollection();
}

void VirtualMachine::addGlobal(const std::string &name, const Value global) {
    _globals[name] = global;
}

void VirtualMachine::addGlobals(const Mapping<std::string, Value> &globals) {
    for (const auto &global : globals) {
        _globals[global.first] = global.second;
    }
}

const Mapping<std::string, Value> VirtualMachine::globals() const { return _globals; }
const Mapping<std::string, Value> VirtualMachine::exports() const { return _exports; }

CallFrame &VirtualMachine::frame() { return _frames.back(); }

static inline Opcode Read(Bytecode::Iterator &it) { return *it++; }

static inline uint16_t ReadConstant(Bytecode::Iterator &it) {
    uint8_t high = RawValue(*it++);
    uint8_t low = RawValue(*it++);
    return (high << 8) | low;
}

static inline uint16_t ReadJump(Bytecode::Iterator &it) {
    uint8_t high = RawValue(*it++);
    uint8_t low = RawValue(*it++);
    return (high << 8) | low;
}

template <typename Stack> static inline typename Stack::value_type Pop(Stack &stack) {
    auto value = stack.back();
    stack.pop_back();
    return value;
}

static inline Value Peek(std::vector<Value> &stack) { return stack.back(); }

static inline void Push(std::vector<Value> &stack, const Value &value) { stack.push_back(value); }

#define BINARY(OP)                                                                         \
    auto rhs = Pop(_stack);                                                                \
    auto lhs = Pop(_stack);                                                                \
    if (lhs.isInteger() && rhs.isInteger()) {                                              \
        Push(_stack, lhs.asInteger() OP rhs.asInteger());                                  \
    } else if (lhs.isNumber() && rhs.isNumber()) {                                         \
        Push(_stack, lhs.castFloat() OP rhs.castFloat());                                  \
    } else {                                                                               \
        error = Error(frame().bytecode->location(frame().ip - 1), Errors::MismatchedTypes, \
                      lhs.typeName(), #OP, rhs.typeName());                                \
        break;                                                                             \
    }

#if defined(DEBUG)
std::ostream &operator<<(std::ostream &out, const CallFrame &f) { return out << f.sp; }
#endif

Result<Value, Error> VirtualMachine::execute(const Strong<Bytecode> &bytecode) {
    _frames.push_back(CallFrame(bytecode, {}, 0));
    frame().it = _it;
    Push(_stack, Value());
    auto localsCount = frame().bytecode->locals().size();
    for (auto i = 0; i < localsCount; i++) {
        Push(_stack, Value());
    }
#if defined(DEBUG)
    if (config.enableTracing) {
        std::cout << "[" << _stack << "]" << std::endl;
        if (_frames.size() > 1) {
            std::cout << "[" << Join(_frames, ", ") << "]" << std::endl;
        }
        std::cout << std::endl;
    }
#endif
    while (true) {
        Optional<Value> returnValue;
        Optional<Error> error;
#if defined(DEBUG)
        if (config.enableTracing) {
            std::cout << frame().bytecode->decodePosition(frame().ip) << " ";
            frame().bytecode->disassemble(std::cout, frame().ip);
        }
#endif
        switch (Read(frame().ip)) {
        case Opcode::Return: {
            auto value = Pop(_stack);
            while (_stack.size() > frame().sp) {
                Pop(_stack);
            }
            _frames.pop_back();
            Push(_stack, value);
            if (_frames.empty()) {
                returnValue = value;
            }
            break;
        }
        case Opcode::Jump: {
            auto offset = ReadJump(frame().ip);
            frame().ip += offset;
            break;
        }
        case Opcode::JumpIfFalse: {
            auto offset = ReadJump(frame().ip);
            auto value = Peek(_stack);
            if (!value.isBool()) {
                error =
                    Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedTrueOrFalse);
                break;
            }
            if (!value.asBool()) {
                frame().ip += offset;
            }
            break;
        }
        case Opcode::JumpIfTrue: {
            auto offset = ReadJump(frame().ip);
            auto value = Peek(_stack);
            if (!value.isBool()) {
                error =
                    Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedTrueOrFalse);
                break;
            }
            if (value.asBool()) {
                frame().ip += offset;
            }
            break;
        }
        case Opcode::JumpIfAtEnd: {
            auto offset = ReadJump(frame().ip);
            auto enumerator = Peek(_stack).as<Enumerator>();
            if (!enumerator) {
                error =
                    Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedEnumerator);
                break;
            }
            if (enumerator->isAtEnd()) {
                frame().ip += offset;
            }
            break;
        }
        case Opcode::PushJump: {
            auto location = ReadJump(frame().ip);
            frame().jumps.push_back(frame().bytecode->code().begin() + location);
            frame().sps.push_back(_stack.size());
            frame().error = Value();
            break;
        }
        case Opcode::PopJump: {
            frame().jumps.pop_back();
            break;
        }
        case Opcode::Repeat: {
            auto offset = ReadJump(frame().ip);
            frame().ip -= offset;
            break;
        }
        case Opcode::Pop: {
            Pop(_stack);
            break;
        }
        case Opcode::Constant: {
            auto index = ReadConstant(frame().ip);
            auto constant = frame().bytecode->constants()[index];
            if (auto copyable = constant.as<Copyable>()) {
                Push(_stack, copyable->copy(*this));
            } else {
                Push(_stack, constant);
            }
            break;
        }
        case Opcode::Short: {
            Push(_stack, ReadConstant(frame().ip));
            break;
        }
        case Opcode::GetEnumerator: {
            auto value = Pop(_stack);
            auto enumerable = value.as<Enumerable>();
            if (!enumerable) {
                error = Error(frame().bytecode->location(frame().ip - 1),
                              Errors::ExpectedListStringDictRange);
                break;
            }
            auto enumeratorValue = enumerable->enumerator(value);
            Push(_stack, enumeratorValue);
            if (auto enumerator = enumeratorValue.as<Enumerator>()) {
                trackContainer(std::static_pointer_cast<Object>(enumerator));
            }
            break;
        }
        case Opcode::SetGlobal: {
            auto index = ReadConstant(frame().ip);
            auto name = frame().bytecode->constants()[index];
            _exports[name.as<String>()->string()] = Pop(_stack);
            break;
        }
        case Opcode::GetGlobal: {
            auto index = ReadConstant(frame().ip);
            auto nameValue = frame().bytecode->constants()[index];
            auto name = nameValue.as<String>()->string();

            auto it = _exports.find(name);
            if (it != _exports.end()) {
                Push(_stack, it->second);
                break;
            }
            it = _globals.find(name);
            if (it != _globals.end()) {
                Push(_stack, it->second);
                break;
            }
            Push(_stack, Value());
            break;
        }
        case Opcode::SetLocal: {
            auto index = ReadConstant(frame().ip);
            _stack[frame().sp + index] = Pop(_stack);
            break;
        }
        case Opcode::GetLocal: {
            auto index = ReadConstant(frame().ip);
            Push(_stack, _stack[frame().sp + index]);
            break;
        }
        case Opcode::SetCapture: {
            auto index = ReadConstant(frame().ip);
            _stack[frame().captures[index]] = Pop(_stack);
            break;
        }
        case Opcode::GetCapture: {
            auto index = ReadConstant(frame().ip);
            Push(_stack, _stack[frame().captures[index]]);
            break;
        }
        case Opcode::OpenRange: {
            auto end = Pop(_stack);
            auto start = Pop(_stack);
            if (range(start, end, false)) {
                break;
            }
            break;
        }
        case Opcode::ClosedRange: {
            auto end = Pop(_stack);
            auto start = Pop(_stack);
            if (range(start, end, true)) {
                break;
            }
            break;
        }
        case Opcode::List: {
            const auto count = ReadConstant(frame().ip);
            std::vector<Value> values(count);
            for (size_t i = 0; i < count; i++) {
                values[count - i - 1] = Pop(_stack);
            }
            auto list = make<List>(values);
            Push(_stack, list);
            break;
        }
        case Opcode::UnpackList: {
            auto count = ReadConstant(frame().ip);
            auto value = Pop(_stack);
            auto list = value.as<List>();
            if (!list) {
                error = Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedList,
                              value.typeName());
                break;
            }
            if (list->size() != count) {
                error = Error(frame().bytecode->location(frame().ip - 1),
                              Errors::UnpackListMismatch, count, list->size());
                break;
            }
            for (auto &&value : list->values()) {
                Push(_stack, value);
            }
            break;
        }
        case Opcode::Dictionary: {
            const auto count = ReadConstant(frame().ip);
            ValueMap values(count);
            for (size_t i = 0; i < count; i++) {
                auto value = Pop(_stack);
                auto key = Pop(_stack);
                values[key] = value;
            }
            auto dictionary = make<Dictionary>(values);
            Push(_stack, dictionary);
            break;
        }
        case Opcode::Negate: {
            auto value = Pop(_stack);
            if (value.isInteger()) {
                Push(_stack, -value.asInteger());
            } else if (value.isFloat()) {
                Push(_stack, -value.asFloat());
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedNumber,
                              value.typeName());
                break;
            }
            break;
        }
        case Opcode::Not: {
            auto value = Pop(_stack);
            if (!value.isBool()) {
                error =
                    Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedTrueOrFalse);
                break;
            }
            Push(_stack, !value.asBool());
            break;
        }
        case Opcode::Increment: {
            auto value = Pop(_stack);
            Push(_stack, value.asInteger() + 1);
            break;
        }
        case Opcode::Add: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);

            // Only allow string concatenation between strings
            if (lhs.isString() && rhs.isString()) {
                std::ostringstream ss;
                ss << lhs << rhs;
                Push(_stack, ss.str());
            } else if (lhs.isInteger() && rhs.isInteger()) {
                Push(_stack, lhs.asInteger() + rhs.asInteger());
            } else if (lhs.isNumber() && rhs.isNumber()) {
                Push(_stack, lhs.castFloat() + rhs.castFloat());
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1), Errors::MismatchedTypes,
                              lhs.typeName(), "+", rhs.typeName());
            }
            break;
        }
        case Opcode::Subtract: {
            BINARY(-);
            break;
        }
        case Opcode::Multiply: {
            BINARY(*);
            break;
        }
        case Opcode::Divide: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            if (lhs.isInteger() && rhs.isInteger()) {
                if (rhs.asInteger() == 0) {
                    error = Error(frame().bytecode->location(frame().ip - 1), Errors::DivideByZero);
                    break;
                }
                Push(_stack, lhs.asInteger() / rhs.asInteger());
            } else if (lhs.isNumber() && rhs.isNumber()) {
                float denom = rhs.castFloat();
                if (denom == 0.0) {
                    error = Error(frame().bytecode->location(frame().ip - 1), Errors::DivideByZero);
                    break;
                }
                Push(_stack, lhs.castFloat() / denom);
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1), Errors::MismatchedTypes,
                              lhs.typeName(), "/", rhs.typeName());
                break;
            }
            break;
        }
        case Opcode::Exponent: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            if (lhs.isNumber() && rhs.isNumber()) {
                Push(_stack, std::pow(lhs.castFloat(), rhs.castFloat()));
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1), Errors::MismatchedTypes,
                              lhs.typeName(), "^", rhs.typeName());
                break;
            }
            break;
        }
        case Opcode::Modulo: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            if (lhs.isInteger() && rhs.isInteger()) {
                Push(_stack, lhs.asInteger() % rhs.asInteger());
            } else if (lhs.isNumber() && rhs.isNumber()) {
                Push(_stack, std::fmod(lhs.castFloat(), rhs.castFloat()));
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1), Errors::MismatchedTypes,
                              lhs.typeName(), "%", rhs.typeName());
                break;
            }
            break;
        }
        case Opcode::Equal: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            Push(_stack, lhs == rhs);
            break;
        }
        case Opcode::NotEqual: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            Push(_stack, !(lhs == rhs));
            break;
        }
        case Opcode::LessThan: {
            BINARY(<);
            break;
        }
        case Opcode::GreaterThan: {
            BINARY(>);
            break;
        }
        case Opcode::LessThanOrEqual: {
            BINARY(<=);
            break;
        }
        case Opcode::GreaterThanOrEqual: {
            BINARY(>=);
            break;
        }
        case Opcode::Subscript: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            if (auto subscriptable = lhs.as<Subscriptable>()) {
                if (auto result = subscriptable->subscript(
                        *this, frame().bytecode->location(frame().ip - 1), rhs)) {
                    Push(_stack, result.value());
                } else {
                    error = result.error();
                    break;
                }
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1),
                              Errors::ExpectedListStringDictRange);
                break;
            }
            break;
        }
        case Opcode::SetSubscript: {
            auto subscript = Pop(_stack);
            auto target = Pop(_stack);
            auto value = Pop(_stack);
            if (auto subscriptable = target.as<Subscriptable>()) {
                auto result = subscriptable->setSubscript(
                    *this, frame().bytecode->location(frame().ip - 1), subscript, value);
                if (!result) {
                    error = result.error();
                    break;
                }
            } else {
                error = Error(frame().bytecode->location(frame().ip - 1),
                              Errors::ExpectedListStringDictRange);
                break;
            }
            break;
        }
        case Opcode::Enumerate: {
            auto value = Peek(_stack);
            Push(_stack, value.as<Enumerator>()->enumerate());
            break;
        }
        case Opcode::True: {
            Push(_stack, true);
            break;
        }
        case Opcode::False: {
            Push(_stack, false);
            break;
        }
        case Opcode::Call: {
            auto callLocation = frame().ip - frame().bytecode->code().begin() - 1;
            auto count = ReadConstant(frame().ip);
            auto object = _stack.end()[-count - 1];
            auto ranges = frame().bytecode->argumentRanges(callLocation);
            error = call(object, count, ranges);
            break;
        }
        case Opcode::Empty: {
            Push(_stack, Value());
            break;
        }
        case Opcode::SetIt: {
            frame().it = Pop(_stack);
            break;
        }
        case Opcode::GetIt: {
            Push(_stack, frame().it);
            break;
        }
        case Opcode::Show: {
            std::cout << Peek(_stack) << std::endl;
            break;
        }
        case Opcode::ToString: {
            auto value = Pop(_stack);
            std::ostringstream ss;
            ss << value;
            Push(_stack, ss.str());
            break;
        }
        }
#if defined(DEBUG)
        if (config.enableTracing) {
            std::cout << std::endl << "[" << _stack << "]" << std::endl;
            if (_frames.size() > 1) {
                std::cout << "[" << Join(_frames, ", ") << "]" << std::endl;
            }
            std::cout << std::endl;
        }
#endif
        if (_haltRequested) {
            error = Error(frame().bytecode->location(frame().ip), Errors::ProgramHalted);
            runPendingGarbageCollection();
            return Fail(error.value());
        }
        if (error.has_value()) {
            if (frame().sps.size() > 0) {
                auto sp = Pop(frame().sps);
                while (_stack.size() > sp) {
                    Pop(_stack);
                }
            }
            while (_frames.size() > 1 && frame().jumps.size() == 0) {
                while (_stack.size() > frame().sp) {
                    Pop(_stack);
                }
                _frames.pop_back();
            }
            if (frame().jumps.size() == 0) {
                runPendingGarbageCollection();
                return Fail(error.value());
            }
            frame().error = error.value().value;
            frame().ip = Pop(frame().jumps);
        }
        if (returnValue.has_value()) {
            _stack.clear();
            runPendingGarbageCollection();
            return returnValue.value();
        }

        maybeTriggerGarbageCollection();
    }
}

void VirtualMachine::requestHalt() { _haltRequested = true; }

Optional<Error> VirtualMachine::call(Value object, int count, std::vector<SourceRange> ranges) {
    if (auto fn = object.as<Function>()) {
        std::vector<size_t> captures;
        for (auto capture : fn->captures()) {
            if (capture.isLocal) {
                captures.push_back(frame().sp + capture.index);
            } else {
                captures.push_back(frame().captures[capture.index]);
            }
        }
        auto sp = _stack.size() - count - 1;
        _frames.push_back(CallFrame(fn->bytecode(), captures, sp));

        auto additionalLocalsCount = frame().bytecode->locals().size() - count;
        for (auto i = 0; i < additionalLocalsCount; i++) {
            Push(_stack, Value());
        }
    } else if (auto native = object.as<Native>()) {
        auto location = frame().bytecode->location(frame().ip - 3);
        auto args = &_stack.end()[-count];

        NativeCallContext context(*this, location, args, ranges);

        auto previousInNative = _inNativeCall;
        auto startIndex = _transientRoots.size();
        _inNativeCall = true;
        [[maybe_unused]] auto guard = Defer([this, startIndex, previousInNative]() {
            _transientRoots.resize(startIndex);
            _inNativeCall = previousInNative;
            if (!_inNativeCall && _gcPending) {
                runPendingGarbageCollection();
            }
        });

        auto result = native->callable()(context);

        _stack.erase(_stack.end() - count - 1, _stack.end());
        if (result) {
            Push(_stack, result.value());
            return None;
        } else {
            return result.error();
        }
    } else {
        return Error(frame().bytecode->location(frame().ip - 3), Errors::UnexpectedTypeForCall);
    }
    return None;
}

Optional<Error> VirtualMachine::range(Value start, Value end, bool closed) {
    if (!start.isInteger()) {
        return Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedInteger);
    }
    if (!end.isInteger()) {
        return Error(frame().bytecode->location(frame().ip - 1), Errors::ExpectedInteger);
    }
    if (end.asInteger() < start.asInteger()) {
        return Error(frame().bytecode->location(frame().ip - 1), Errors::BoundsMismatch);
    }
    Push(_stack, MakeStrong<Range>(start.asInteger(), end.asInteger(), closed));
    return None;
}

#pragma mark - Garbage Collection

std::vector<Strong<Object>> VirtualMachine::gatherRootObjects() const {
    std::vector<Strong<Object>> roots;

    // Add objects from globals
    for (const auto &pair : _globals) {
        if (pair.second.isObject()) {
            roots.push_back(pair.second.asObject());
        }
    }

    // Add objects from exports
    for (const auto &pair : _exports) {
        if (pair.second.isObject()) {
            roots.push_back(pair.second.asObject());
        }
    }

    // Add objects from the VM stack
    for (const auto &value : _stack) {
        if (value.isObject()) {
            roots.push_back(value.asObject());
        }
    }

    // Add objects from the "it" variable
    if (_it.isObject()) {
        roots.push_back(_it.asObject());
    }

    // Add objects from call frames
    for (const auto &frame : _frames) {
        if (frame.error.isObject()) {
            roots.push_back(frame.error.asObject());
        }
        if (frame.it.isObject()) {
            roots.push_back(frame.it.asObject());
        }
    }

    for (const auto &weak : _transientRoots) {
        if (auto object = weak.lock()) {
            roots.push_back(std::move(object));
        }
    }

    return roots;
}

void VirtualMachine::notifyContainerMutation(List *list) {
    assert(list && "notifyContainerMutation called with null List");
    accountForContainer(list, estimateContainerSize(list), true);
    maybeTriggerGarbageCollection();
}

void VirtualMachine::notifyContainerMutation(Dictionary *dictionary) {
    assert(dictionary && "notifyContainerMutation called with null Dictionary");
    accountForContainer(dictionary, estimateContainerSize(dictionary), true);
    maybeTriggerGarbageCollection();
}

void VirtualMachine::serviceGarbageCollection() {
    if (_gcInProgress) {
        return;
    }
    _gcPending = true;
    runPendingGarbageCollection();
}

void VirtualMachine::trackContainer(const Strong<Object> &container) {
    auto *object = container.get();
    if (_trackedContainers.find(object) != _trackedContainers.end()) {
        return;
    }
    _trackedContainers[object] = container;
    accountForContainer(object, estimateContainerSize(object), true);
    maybeTriggerGarbageCollection();
}

size_t VirtualMachine::estimateContainerSize(const Object *object) const {
    if (auto list = dynamic_cast<const List *>(object)) {
        const auto &values = list->values();
        return sizeof(List) + values.capacity() * sizeof(Value);
    }
    if (auto dictionary = dynamic_cast<const Dictionary *>(object)) {
        const auto &values = dictionary->values();
        size_t elementBytes = values.size() * sizeof(ValueMap::value_type);
        size_t bucketBytes = values.bucket_count() * sizeof(void *);
        return sizeof(Dictionary) + elementBytes + bucketBytes;
    }
    return 0;
}

void VirtualMachine::cleanupExpiredContainers() {
    std::vector<Object *> expired;
    expired.reserve(_trackedContainers.size());
    for (auto &entry : _trackedContainers) {
        if (entry.second.expired()) {
            expired.push_back(entry.first);
        }
    }
    for (auto *ptr : expired) {
        deregisterContainer(ptr);
    }
}

void VirtualMachine::deregisterContainer(Object *object) {
    auto sizeIt = _containerSizes.find(object);
    if (sizeIt != _containerSizes.end()) {
        size_t previous = sizeIt->second;
        if (_liveContainerBytes >= previous) {
            _liveContainerBytes -= previous;
        } else {
            _liveContainerBytes = 0;
        }
        _containerSizes.erase(sizeIt);
    }
    _trackedContainers.erase(object);
}

void VirtualMachine::accountForContainer(Object *object, size_t newSize, bool accumulateDebt) {
    assert(object);
    if (_trackedContainers.find(object) == _trackedContainers.end()) {
        return;
    }
    auto &current = _containerSizes[object];
    size_t previous = current;
    if (newSize >= previous) {
        size_t delta = newSize - previous;
        if (delta > 0) {
            _liveContainerBytes += delta;
            if (accumulateDebt) {
                _bytesSinceLastGc += delta;
            }
        }
    } else {
        size_t delta = previous - newSize;
        if (_liveContainerBytes >= delta) {
            _liveContainerBytes -= delta;
        } else {
            _liveContainerBytes = 0;
        }
    }
    current = newSize;
    if (_nextGcThreshold == 0) {
        _nextGcThreshold = std::max(config.initialGarbageCollectionThresholdBytes,
                                    config.minimumGarbageCollectionThresholdBytes);
    }
}

void VirtualMachine::refreshContainerMetrics(bool accumulateDebt) {
    cleanupExpiredContainers();
    for (auto &entry : _trackedContainers) {
        if (auto locked = entry.second.lock()) {
            accountForContainer(entry.first, estimateContainerSize(locked.get()), accumulateDebt);
        }
    }
}

void VirtualMachine::maybeTriggerGarbageCollection() {
    if (_gcInProgress) {
        return;
    }
    if (_nextGcThreshold == 0 || _bytesSinceLastGc < _nextGcThreshold) {
        return;
    }
    _gcPending = true;
    runPendingGarbageCollection();
}

void VirtualMachine::runPendingGarbageCollection() {
    if (!_gcPending || _gcInProgress) {
        return;
    }

    // Clear out expired weak references so the collector only considers live candidates.
    cleanupExpiredContainers();
    if (_gcInProgress) {
        return;
    }

    _gcInProgress = true;

    size_t previousCount = _garbageCollectionCount;

    // Hold strong references to every tracked container during the collection.
    std::vector<Strong<Object>> strongRefs;
    strongRefs.reserve(_trackedContainers.size());

    // Snapshot every root we can reach (stack, globals, frames, transient native roots, etc.).
    auto roots = gatherRootObjects();

    std::vector<Object *> expired;
    for (auto &entry : _trackedContainers) {
        auto locked = entry.second.lock();
        if (!locked) {
            expired.push_back(entry.first);
            continue;
        }
        locked->visited = false;
        strongRefs.push_back(std::move(locked));
    }

    for (auto *ptr : expired) {
        deregisterContainer(ptr);
    }

    if (!strongRefs.empty()) {
        // Depth-first mark over the object graph starting from the root set.
        static const auto markReachable = [](Object *root) {
            std::stack<Object *> stack;
            stack.push(root);
            while (!stack.empty()) {
                auto *current = stack.top();
                stack.pop();
                if (current->visited) {
                    continue;
                }
                current->visited = true;
                current->trace([&stack](Strong<Object> &child) {
                    if (child && !child->visited) {
                        stack.push(child.get());
                    }
                });
            }
        };

        for (auto &root : roots) {
            if (root) {
                markReachable(root.get());
            }
        }

        // Sweep: drop edges from any container that was not marked reachable.
        for (auto &obj : strongRefs) {
            if (!obj->visited) {
                obj->trace([](Strong<Object> &child) { child.reset(); });
            }
        }

        _garbageCollectionCount++;

        // Refresh size accounting for survivors so thresholds stay accurate.
        for (auto &obj : strongRefs) {
            if (obj && obj->visited) {
                accountForContainer(obj.get(), estimateContainerSize(obj.get()), false);
            }
        }
    }

    _gcInProgress = false;

    if (_garbageCollectionCount > previousCount) {
        double growth = std::max(1.0, config.garbageCollectionGrowthFactor);
        size_t baseline = std::max(config.initialGarbageCollectionThresholdBytes,
                                   config.minimumGarbageCollectionThresholdBytes);
        size_t nextThreshold = baseline;
        if (_liveContainerBytes > 0) {
            nextThreshold = std::max(
                baseline,
                static_cast<size_t>(std::ceil(static_cast<double>(_liveContainerBytes) * growth)));
        }
        _nextGcThreshold = nextThreshold;
        _bytesSinceLastGc = 0;
    }

    _gcPending = false;
    cleanupExpiredContainers();
}

SIF_NAMESPACE_END
