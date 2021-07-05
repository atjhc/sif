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
#include "runtime/objects/Range.h"
#include "runtime/objects/String.h"

SIF_NAMESPACE_BEGIN

VirtualMachine::VirtualMachine(const VirtualMachineConfig &config) : _config(config) {}

Optional<RuntimeError> VirtualMachine::error() const { return _error; }

void VirtualMachine::add(const std::string &name, const Strong<Native> &nativeFunction) {
    _variables[name] = nativeFunction;
}

VirtualMachine::CallFrame &VirtualMachine::frame() { return _callStack.back(); }

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

static inline Value Pop(std::vector<Value> &stack) {
    auto value = stack.back();
    stack.pop_back();
    return value;
}

static inline Value Peek(std::vector<Value> &stack) { return stack.back(); }

static inline void Push(std::vector<Value> &stack, const Value &value) { stack.push_back(value); }

#define BINARY(OP)                                                                             \
    auto rhs = Pop(_stack);                                                                    \
    auto lhs = Pop(_stack);                                                                    \
    if (lhs.isInteger() && rhs.isInteger()) {                                                  \
        Push(_stack, lhs.asInteger() OP rhs.asInteger());                                      \
    } else if (lhs.isFloat() && rhs.isFloat()) {                                               \
        Push(_stack, lhs.asFloat() OP rhs.asFloat());                                          \
    } else {                                                                                   \
        _error = RuntimeError(frame().bytecode->location(frame().ip - 1), "mismatched types"); \
        return None;                                                                           \
    }

#if defined(DEBUG)
std::ostream &operator<<(std::ostream &out, const VirtualMachine::CallFrame &f) {
    return out << f.sp;
}
#endif

Optional<Value> VirtualMachine::execute(const Strong<Bytecode> &bytecode) {
    _error = None;
    _callStack.push_back({bytecode, bytecode->code().begin(), 0});
    Push(_stack, Value());
    while (true) {
#if defined(DEBUG)
        if (_config.enableTracing) {
            frame().bytecode->disassemble(std::cout, frame().ip);
            std::cout << std::endl << "[" << _stack << "]" << std::endl;
            std::cout << "[" << Join(_callStack, ", ") << "]" << std::endl;
            std::cout << std::string(5, '-') << std::endl;
        }
#endif
        switch (Opcode instruction = Read(frame().ip)) {
        case Opcode::Return: {
            auto value = Pop(_stack);
            while (_stack.size() > _callStack.back().sp) {
                Pop(_stack);
            }
            _callStack.pop_back();
            if (_callStack.empty()) {
                return value;
            }
            Push(_stack, value);
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
                _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                      "expected true or false");
                return None;
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
                _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                      "expected true or false");
                return None;
            }
            if (value.asBool()) {
                frame().ip += offset;
            }
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
            Push(_stack, frame().bytecode->constants()[index]);
            break;
        }
        case Opcode::Short: {
            Push(_stack, ReadConstant(frame().ip));
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
        case Opcode::OpenRange: {
            auto end = Pop(_stack);
            auto start = Pop(_stack);
            if (range(start, end, false)) {
                return None;
            }
            break;
        }
        case Opcode::ClosedRange: {
            auto end = Pop(_stack);
            auto start = Pop(_stack);
            if (range(start, end, true)) {
                return None;
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
                _error =
                    RuntimeError(frame().bytecode->location(frame().ip - 1), "expected a number");
                return None;
            }
            break;
        }
        case Opcode::Not: {
            auto value = Pop(_stack);
            if (!value.isBool()) {
                _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                      "expected true or false");
                return None;
            }
            Push(_stack, !value.asBool());
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
                    _error =
                        RuntimeError(frame().bytecode->location(frame().ip - 1), "divide by zero");
                    return None;
                }
                Push(_stack, lhs.asInteger() / rhs.asInteger());
            } else if (lhs.isFloat() && rhs.isFloat()) {
                if (rhs.asFloat() == 0.0) {
                    _error =
                        RuntimeError(frame().bytecode->location(frame().ip - 1), "divide by zero");
                    return None;
                }
                Push(_stack, lhs.asFloat() / rhs.asFloat());
            } else {
                _error =
                    RuntimeError(frame().bytecode->location(frame().ip - 1), "mismatched types");
                return None;
            }
            break;
        }
        case Opcode::Exponent: {
            auto lhs = Pop(_stack);
            auto rhs = Pop(_stack);
            if (lhs.isNumber() && rhs.isNumber()) {
                Push(_stack, pow(lhs.castFloat(), rhs.castFloat()));
            } else {
                _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                      "expected floating point values");
                return None;
            }
            break;
        }
        case Opcode::Modulo: {
            auto rhs = Pop(_stack);
            auto lhs = Pop(_stack);
            if (lhs.isInteger() && rhs.isInteger()) {
                Push(_stack, lhs.asInteger() % rhs.asInteger());
            } else if (lhs.isFloat() && rhs.isFloat()) {
                Push(_stack, fmod(lhs.asFloat(), rhs.asFloat()));
            } else {
                _error =
                    RuntimeError(frame().bytecode->location(frame().ip - 1), "mismatched types");
                return None;
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
            if (subscript(lhs, rhs)) {
                return None;
            }
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
            if (call(object, count)) {
                return None;
            }
            break;
        }
        case Opcode::Empty: {
            Push(_stack, Value());
            break;
        }
        case Opcode::It: {
            _variables["it"] = Pop(_stack);
            break;
        }
        case Opcode::Show: {
            std::cout << Peek(_stack) << std::endl;
            break;
        }
        }
    }
}

bool VirtualMachine::call(Value object, int count) {
    if (auto fn = object.as<Function>()) {
        _callStack.push_back(
            {fn->bytecode(), fn->bytecode()->code().begin(), _stack.size() - count - 1});
    } else if (auto native = object.as<Native>()) {
        try {
            auto result = native->callable()(&_stack.end()[-count]);
            _stack.erase(_stack.end() - count - 1, _stack.end());
            Push(_stack, result);
        } catch (const RuntimeError &error) {
            _error = error;
            return true;
        } catch (...) {
            _error = RuntimeError(frame().bytecode->location(frame().ip - 3),
                                  "an unexpected error occurred");
            return true;
        }
    } else {
        _error = RuntimeError(frame().bytecode->location(frame().ip - 3),
                              "unexpected type for function call");
        return true;
    }
    return false;
}

bool VirtualMachine::subscript(Value lhs, Value rhs) {
    if (auto list = lhs.as<List>()) {
        if (auto range = rhs.as<Range>()) {
            Push(_stack, (*list)[*range]);
            return false;
        }
        if (rhs.isInteger()) {
            auto index = rhs.asInteger();
            if (index >= static_cast<int>(list->values().size()) ||
                static_cast<int>(list->values().size()) + index < 0) {
                _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                      "array index out of bounds");
                return true;
            }
            Push(_stack, list->values()[index < 0 ? list->values().size() + index : index]);
            return false;
        }
        _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                              "expected an integer or range");
        return true;
    } else if (auto dictionary = lhs.as<Dictionary>()) {
        auto it = dictionary->values().find(rhs);
        if (it == dictionary->values().end()) {
            Push(_stack, Value());
        } else {
            Push(_stack, dictionary->values().at(rhs));
        }
    } else if (auto string = lhs.as<String>()) {
        if (!rhs.isInteger()) {
            _error =
                RuntimeError(frame().bytecode->location(frame().ip - 1), "expected an integer");
            return true;
        }
        auto index = rhs.asInteger();
        if (index >= static_cast<int>(string->string().size()) ||
            static_cast<int>(string->string().size()) + index < 0) {
            _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                                  "string index out of bounds");
            return true;
        }
        Push(_stack,
             string->string().substr(index < 0 ? string->string().size() + index : index, 1));
    } else {
        _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                              "expected a list, string, or dictionary");
        return true;
    }
    return false;
}

bool VirtualMachine::range(Value start, Value end, bool closed) {
    if (!start.isInteger()) {
        _error = RuntimeError(frame().bytecode->location(frame().ip - 1), "expected an integer");
        return true;
    }
    if (!end.isInteger()) {
        _error = RuntimeError(frame().bytecode->location(frame().ip - 1), "expected an integer");
        return true;
    }
    if (end.asInteger() < start.asInteger()) {
        _error = RuntimeError(frame().bytecode->location(frame().ip - 1),
                              "lower bound must be less than or equal to the upper bound");
        return true;
    }
    Push(_stack, MakeStrong<Range>(start.asInteger(), end.asInteger(), closed));
    return false;
}

SIF_NAMESPACE_END