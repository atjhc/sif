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

#include "parser/VirtualMachine.h"
#include "runtime/objects/String.h"
#include "runtime/objects/List.h"

CH_NAMESPACE_BEGIN

VirtualMachine::VirtualMachine(const VirtualMachineConfig &config) : _config(config) {}

Optional<RuntimeError> VirtualMachine::error() const {
    return _error;
}

static inline Opcode Read(Bytecode::Iterator &it) {
    return *it++;
}

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

static inline Value Pop(std::stack<Value> &stack) {
    auto value = stack.top();
    stack.pop();
    return value;
}

#define BINARY(OP)                                              \
    auto rhs = Pop(_stack);                                     \
    auto lhs = Pop(_stack);                                     \
    if (lhs.isInteger() && rhs.isInteger()) {                   \
        _stack.push(lhs.asInteger() OP rhs.asInteger());        \
    } else if (lhs.isFloat() && rhs.isFloat()) {                \
        _stack.push(lhs.asFloat() OP rhs.asFloat());            \
    } else {                                                    \
        _error = RuntimeError(bytecode->location(_ip), "mismatched types"); \
        return Empty;                                           \
    }

Optional<Value> VirtualMachine::execute(const Strong<Bytecode> &bytecode) {
    _error = Empty;
    _ip = bytecode->code().begin();
    while (true) {
        switch (Opcode instruction = Read(_ip)) {
            case Opcode::Return: {
                if (_stack.size()) {
                    return Pop(_stack);
                }
                return Empty;
            }
            case Opcode::Jump: {
                auto offset = ReadJump(_ip);
                _ip += offset;
                break;
            }
            case Opcode::JumpIfFalse: {
                auto offset = ReadJump(_ip);
                auto value = _stack.top();
                if (!value.isBool()) {
                    _error = RuntimeError(bytecode->location(_ip), "expected bool type");
                    return Empty;
                }
                if (!value.asBool()) {
                    _ip += offset;
                }
                break;
            }
            case Opcode::JumpIfTrue: {
                auto offset = ReadJump(_ip);
                auto value = _stack.top();
                if (!value.isBool()) {
                    _error = RuntimeError(bytecode->location(_ip), "expected bool type");
                    return Empty;
                }
                if (value.asBool()) {
                    _ip += offset;
                }
                break;
            }
            case Opcode::Repeat: {
                auto offset = ReadJump(_ip);
                _ip -= offset;
                break;
            }
            case Opcode::Pop: {
                Pop(_stack);
                break;
            }
            case Opcode::Constant: {
                auto index = ReadConstant(_ip);
                _stack.push(bytecode->constants().at(index));
                break;
            }
            case Opcode::Short: {
                _stack.push(ReadConstant(_ip));
                break;
            }
            case Opcode::SetVariable: {
                auto index = ReadConstant(_ip);
                auto name = bytecode->constants()[index];
                _variables[name.as<String>()->string()] = Pop(_stack);
                break;
            }
            case Opcode::GetVariable: {
                auto index = ReadConstant(_ip);
                auto name = bytecode->constants()[index];
                auto it = _variables.find(name.as<String>()->string());
                if (it == _variables.end()) {
                    _stack.push(Value());
                } else {
                    _stack.push(it->second);
                }
                break;
            }
            case Opcode::List: {
                auto count = ReadConstant(_ip);
                std::vector<Value> values(count);
                for (size_t i = 0; i < count; i++) {
                    values[count - i - 1] = Pop(_stack);
                }
                _stack.push(MakeStrong<List>(values));
                break;
            }
            case Opcode::Negate: {
                auto value = Pop(_stack);
                if (value.isInteger()) {
                    _stack.push(-value.asInteger());
                } else if (value.isFloat()) {
                    _stack.push(-value.asFloat());
                } else {
                    _error = RuntimeError(bytecode->location(_ip), "expected numerical type");
                    return Empty;
                }
                break;
            }
            case Opcode::Not: {
                auto value = Pop(_stack);
                if (value.isBool()) {
                    _stack.push(!value.asBool());
                }
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
                BINARY(/);
                break;
            }
            case Opcode::Exponent: {
                auto lhs = Pop(_stack);
                auto rhs = Pop(_stack);
                if (lhs.isNumber() && rhs.isNumber()) {
                    _stack.push(pow(lhs.castFloat(), rhs.castFloat()));
                } else {
                    _error = RuntimeError(bytecode->location(_ip), "expected floating point values");
                    return Empty;
                }
                break;
            }
            case Opcode::Modulo: {
                auto lhs = Pop(_stack);
                auto rhs = Pop(_stack);
                if (lhs.isInteger() && rhs.isInteger()) {
                    _stack.push(lhs.asInteger() % rhs.asInteger());
                } else if (lhs.isFloat() && rhs.isFloat()) {
                    _stack.push(fmod(lhs.asFloat(), rhs.asFloat()));
                } else {
                    _error = RuntimeError(bytecode->location(_ip), "mismatched types");
                    return Empty;
                }
                break;
            }
            case Opcode::Equal: {
                BINARY(==);
                break;
            }
            case Opcode::NotEqual: {
                BINARY(!=);
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
            case Opcode::True: {
                _stack.push(true);
                break;
            }
            case Opcode::False: {
                _stack.push(false);
                break;
            }
            case Opcode::And: {
                auto lhs = Pop(_stack);
                auto rhs = Pop(_stack);
                if (lhs.isBool() && rhs.isBool()) {
                    _stack.push(lhs.asBool() && rhs.asBool());
                } else {
                    _error = RuntimeError(bytecode->location(_ip), "expected bool types");
                    return Empty;
                }
                break;
            }
            case Opcode::Or: {
                auto lhs = Pop(_stack);
                auto rhs = Pop(_stack);
                if (lhs.isBool() && rhs.isBool()) {
                    _stack.push(lhs.asBool() || rhs.asBool());
                } else {
                    _error = RuntimeError(bytecode->location(_ip), "expected bool types");
                    return Empty;
                }
                break;
            }
            // TODO: remove.
            case Opcode::Show:
                std::cout << _stack.top() << std::endl;
                break;
            case Opcode::Call:
                break;
        }
    }
}

CH_NAMESPACE_END
