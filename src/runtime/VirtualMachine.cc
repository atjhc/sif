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
#include "runtime/objects/Dictionary.h"
#include "runtime/objects/Function.h"
#include "runtime/objects/List.h"
#include "runtime/objects/Native.h"
#include "runtime/objects/Range.h"
#include "runtime/objects/String.h"

#include <cmath>

SIF_NAMESPACE_BEGIN

VirtualMachine::VirtualMachine(const VirtualMachineConfig &config) : _config(config) {}

Optional<RuntimeError> VirtualMachine::error() const { return _error; }

void VirtualMachine::add(const std::string &name, const Value global) { _variables[name] = global; }

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

#define BINARY(OP)                                                                        \
    auto rhs = Pop(_stack);                                                               \
    auto lhs = Pop(_stack);                                                               \
    if (lhs.isInteger() && rhs.isInteger()) {                                             \
        Push(_stack, lhs.asInteger() OP rhs.asInteger());                                 \
    } else if (lhs.isNumber() && rhs.isNumber()) {                                        \
        Push(_stack, lhs.castFloat() OP rhs.castFloat());                                 \
    } else {                                                                              \
        error = RuntimeError(                                                             \
            frame().bytecode->location(frame().ip - 1),                                   \
            Concat("mismatched types: ", lhs.typeName(), " ", #OP, " ", rhs.typeName())); \
        break;                                                                            \
    }

#if defined(DEBUG)
std::ostream &operator<<(std::ostream &out, const CallFrame &f) { return out << f.sp; }
#endif

Optional<Value> VirtualMachine::execute(const Strong<Bytecode> &bytecode) {
    _error = None;
    _frames.push_back(CallFrame(bytecode, {}, 0));
    Push(_stack, Value());
    auto localsCount = frame().bytecode->locals().size();
    for (auto i = 0; i < localsCount; i++) {
        Push(_stack, Value());
    }
#if defined(DEBUG)
    if (_config.enableTracing) {
        std::cout << "[" << _stack << "]" << std::endl;
        if (_frames.size() > 1) {
            std::cout << "[" << Join(_frames, ", ") << "]" << std::endl;
        }
        std::cout << std::endl;
    }
#endif
    while (true) {
        Optional<Value> returnValue;
        Optional<RuntimeError> error;
#if defined(DEBUG)
        if (_config.enableTracing) {
            std::cout << frame().bytecode->decodePosition(frame().ip) << " ";
            frame().bytecode->disassemble(std::cout, frame().ip);
        }
#endif
        switch (Opcode instruction = Read(frame().ip)) {
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
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected true or false");
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
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected true or false");
                return None;
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
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected an enumerator");
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
                Push(_stack, copyable->copy());
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
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected a list, dictionary, string, or range");
                break;
            }
            Push(_stack, enumerable->enumerator(value));
            break;
        }
        case Opcode::SetGlobal: {
            auto index = ReadConstant(frame().ip);
            auto name = frame().bytecode->constants()[index];
            _variables[name.as<String>()->string()] = Pop(_stack);
            break;
        }
        case Opcode::GetGlobal: {
            auto index = ReadConstant(frame().ip);
            auto name = frame().bytecode->constants()[index];
            auto it = _variables.find(name.as<String>()->string());
            if (it == _variables.end()) {
                Push(_stack, Value());
            } else {
                Push(_stack, it->second);
            }
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
            Push(_stack, MakeStrong<List>(values));
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
            Push(_stack, MakeStrong<Dictionary>(values));
            break;
        }
        case Opcode::Negate: {
            auto value = Pop(_stack);
            if (value.isInteger()) {
                Push(_stack, -value.asInteger());
            } else if (value.isFloat()) {
                Push(_stack, -value.asFloat());
            } else {
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     Concat("expected a number, got ", value.typeName()));
                break;
            }
            break;
        }
        case Opcode::Not: {
            auto value = Pop(_stack);
            if (!value.isBool()) {
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected true or false");
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
            BINARY(+);
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
                    error =
                        RuntimeError(frame().bytecode->location(frame().ip - 1), "divide by zero");
                    break;
                }
                Push(_stack, lhs.asInteger() / rhs.asInteger());
            } else if (lhs.isNumber() && rhs.isNumber()) {
                float denom = rhs.castFloat();
                if (denom == 0.0) {
                    error =
                        RuntimeError(frame().bytecode->location(frame().ip - 1), "divide by zero");
                    break;
                }
                Push(_stack, lhs.castFloat() / denom);
            } else {
                error = RuntimeError(
                    frame().bytecode->location(frame().ip - 1),
                    Concat("mismatched types: ", lhs.typeName(), " / ", rhs.typeName()));
                break;
            }
            break;
        }
        case Opcode::Exponent: {
            auto lhs = Pop(_stack);
            auto rhs = Pop(_stack);
            if (lhs.isNumber() && rhs.isNumber()) {
                Push(_stack, std::pow(lhs.castFloat(), rhs.castFloat()));
            } else {
                error = RuntimeError(
                    frame().bytecode->location(frame().ip - 1),
                    Concat("mismatched types: ", lhs.typeName(), " ^ ", rhs.typeName()));
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
                error = RuntimeError(
                    frame().bytecode->location(frame().ip - 1),
                    Concat("mismatched types: ", lhs.typeName(), " % ", rhs.typeName()));
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
                if (auto result =
                        subscriptable->subscript(frame().bytecode->location(frame().ip - 1), rhs)) {
                    Push(_stack, result.value());
                } else {
                    error = result.error();
                    break;
                }
            } else {
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected a list, string, dictionary, or range");
                break;
            }
            break;
        }
        case Opcode::SetSubscript: {
            auto value = Pop(_stack);
            auto subscript = Pop(_stack);
            auto target = Pop(_stack);
            if (auto subscriptable = target.as<Subscriptable>()) {
                auto result = subscriptable->setSubscript(
                    frame().bytecode->location(frame().ip - 1), subscript, value);
                if (!result) {
                    error = result.error();
                    break;
                }
            } else {
                error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                     "expected a list, string, dictionary, or range");
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
            auto count = ReadConstant(frame().ip);
            auto object = _stack.end()[-count - 1];
            error = call(object, count);
            break;
        }
        case Opcode::Empty: {
            Push(_stack, Value());
            break;
        }
        case Opcode::SetIt: {
            _it = Pop(_stack);
            break;
        }
        case Opcode::GetIt: {
            Push(_stack, _it);
            break;
        }
        case Opcode::Show: {
            std::cout << Peek(_stack) << std::endl;
            break;
        }
        }
#if defined(DEBUG)
        if (_config.enableTracing) {
            std::cout << std::endl << "[" << _stack << "]" << std::endl;
            if (_frames.size() > 1) {
                std::cout << "[" << Join(_frames, ", ") << "]" << std::endl;
            }
            std::cout << std::endl;
        }
#endif
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
                _error = error;
                return None;
            }
            frame().error = error.value().value();
            frame().ip = Pop(frame().jumps);
        }
        if (returnValue.has_value()) {
            _stack.clear();
            return returnValue.value();
        }
    }
}

Optional<RuntimeError> VirtualMachine::call(Value object, int count) {
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
        auto result = native->callable()(frame(), frame().bytecode->location(frame().ip - 3),
                                         &_stack.end()[-count]);
        _stack.erase(_stack.end() - count - 1, _stack.end());
        if (result) {
            Push(_stack, result.value());
            return None;
        } else {
            return result.error();
        }
    } else {
        return RuntimeError(frame().bytecode->location(frame().ip - 3),
                            "unexpected type for function call");
    }
    return None;
}

Optional<RuntimeError> VirtualMachine::range(Value start, Value end, bool closed) {
    if (!start.isInteger()) {
        return RuntimeError(frame().bytecode->location(frame().ip - 1), "expected an integer");
    }
    if (!end.isInteger()) {
        return RuntimeError(frame().bytecode->location(frame().ip - 1), "expected an integer");
    }
    if (end.asInteger() < start.asInteger()) {
        return RuntimeError(frame().bytecode->location(frame().ip - 1),
                            "lower bound must be less than or equal to the upper bound");
    }
    Push(_stack, MakeStrong<Range>(start.asInteger(), end.asInteger(), closed));
    return None;
}

SIF_NAMESPACE_END
