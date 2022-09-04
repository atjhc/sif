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

#include "compiler/Bytecode.h"
#include "runtime/objects/Function.h"

#include <climits>
#include <cmath>
#include <iomanip>

SIF_NAMESPACE_BEGIN

size_t Bytecode::add(Location location, Opcode opcode) {
    _code.push_back(opcode);
    _locations.push_back(location);
    return _code.size() - 1;
}

size_t Bytecode::add(Location location, Opcode opcode, uint16_t argument) {
    _code.push_back(opcode);
    _code.push_back(static_cast<Opcode>(argument >> 8));
    _code.push_back(static_cast<Opcode>(argument & 0xff));
    _locations.push_back(location);
    _locations.push_back(location);
    _locations.push_back(location);
    return _code.size() - 3;
}

void Bytecode::addRepeat(Location location, uint16_t argument) {
    auto offset = _code.size() - argument + 3;
    if (offset > USHRT_MAX) {
        throw std::out_of_range(Concat("jump too far (", SHRT_MAX, ")"));
    }
    _code.push_back(Opcode::Repeat);
    _code.push_back(static_cast<Opcode>(offset >> 8));
    _code.push_back(static_cast<Opcode>(offset & 0xff));
    _locations.push_back(location);
    _locations.push_back(location);
    _locations.push_back(location);
}

uint16_t Bytecode::addConstant(Value constant) {
    for (int i = 0; i < _constants.size(); i++) {
        if (_constants[i] == constant) {
            return i;
        }
    }
    _constants.push_back(constant);
    if (_constants.size() > USHRT_MAX) {
        throw std::out_of_range(Concat("too many constants (", USHRT_MAX, ")"));
    }
    return _constants.size() - 1;
}

void Bytecode::patchJump(size_t index) {
    auto offset = _code.size() - index - 3;
    if (offset > USHRT_MAX) {
        throw std::out_of_range(Concat("jump too far (", USHRT_MAX, ")"));
    }
    _code[index + 1] = static_cast<Opcode>(offset >> 8);
    _code[index + 2] = static_cast<Opcode>(offset & 0xff);
}

void Bytecode::patchLocals(size_t location, short count) {
    _code[location + 1] = static_cast<Opcode>(count >> 8);
    _code[location + 2] = static_cast<Opcode>(count & 0xff);
}

const std::vector<Opcode> &Bytecode::code() const { return _code; }

const std::vector<Value> &Bytecode::constants() const { return _constants; }

Location Bytecode::location(Iterator it) const { return _locations[it - _code.begin()]; }

static inline uint16_t ReadUInt16(Bytecode::Iterator position) {
    return RawValue(position[0]) << 8 | RawValue(position[1]);
}

std::string Bytecode::decodePosition(Iterator position) const {
    int width = log10f(code().size()) + 1;
    if (width < 4)
        width = 4;
    std::ostringstream out;
    out << std::setfill('0') << std::setw(width) << position - code().begin();
    return out.str();
}

Bytecode::Iterator Bytecode::disassembleConstant(std::ostream &out, const std::string &name,
                                                 Iterator position) const {
    size_t index = ReadUInt16(position + 1);
    out << name << " " << index << " \"" << _constants.at(index).description() << "\"";
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleList(std::ostream &out, Iterator position) const {
    size_t count = ReadUInt16(position + 1);
    out << "List " << count;
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleDictionary(std::ostream &out, Iterator position) const {
    size_t count = ReadUInt16(position + 1);
    out << "Dictionary " << count;
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleJump(std::ostream &out, const std::string &name,
                                             Iterator position) const {
    size_t offset = ReadUInt16(position + 1);
    out << name << " " << decodePosition(position + offset + 3);
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleRepeat(std::ostream &out, const std::string &name,
                                               Iterator position) const {
    size_t offset = ReadUInt16(position + 1);
    out << name << " " << decodePosition(position - (offset - 3));
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleCall(std::ostream &out, const std::string &name,
                                             Iterator position) const {
    size_t count = ReadUInt16(position + 1);
    out << name << " " << count;
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleShort(std::ostream &out, Iterator position) const {
    uint16_t shortValue = ReadUInt16(position + 1);
    out << "Short " << shortValue;
    return position + 3;
}

Bytecode::Iterator Bytecode::disassembleLocal(std::ostream &out, const std::string &name,
                                              Iterator position) const {
    uint16_t value = ReadUInt16(position + 1);
    out << name << " " << value;
    return position + 3;
}

Bytecode::Iterator Bytecode::disassemble(std::ostream &out, Iterator position) const {
    auto opcode = *position;
    switch (opcode) {
    case Opcode::Jump:
        return disassembleJump(out, "Jump", position);
    case Opcode::JumpIfFalse:
        return disassembleJump(out, "JumpIfFalse", position);
    case Opcode::JumpIfTrue:
        return disassembleJump(out, "JumpIfTrue", position);
    case Opcode::JumpIfAtEnd:
        return disassembleJump(out, "JumpIfAtEnd", position);
    case Opcode::Repeat:
        return disassembleRepeat(out, "Repeat", position);
    case Opcode::Pop:
        out << "Pop";
        return position + 1;
    case Opcode::Return:
        out << "Return";
        return position + 1;
    case Opcode::Locals:
        return disassembleLocal(out, "Locals", position);
    case Opcode::Constant:
        return disassembleConstant(out, "Constant", position);
    case Opcode::Short:
        return disassembleShort(out, position);
    case Opcode::OpenRange:
        out << "OpenRange";
        return position + 1;
    case Opcode::ClosedRange:
        out << "ClosedRange";
        return position + 1;
    case Opcode::List:
        return disassembleList(out, position);
    case Opcode::Dictionary:
        return disassembleDictionary(out, position);
    case Opcode::GetGlobal:
        return disassembleConstant(out, "GetGlobal", position);
    case Opcode::SetGlobal:
        return disassembleConstant(out, "SetGlobal", position);
    case Opcode::GetLocal:
        return disassembleLocal(out, "GetLocal", position);
    case Opcode::SetLocal:
        return disassembleLocal(out, "SetLocal", position);
    case Opcode::GetCapture:
        return disassembleLocal(out, "GetCapture", position);
    case Opcode::SetCapture:
        return disassembleLocal(out, "SetCapture", position);
    case Opcode::Negate:
        out << "Negate";
        return position + 1;
    case Opcode::Not:
        out << "Not";
        return position + 1;
    case Opcode::Enumerate:
        out << "Enumerate";
        return position + 1;
    case Opcode::Increment:
        out << "Increment";
        return position + 1;
    case Opcode::Add:
        out << "Add";
        return position + 1;
    case Opcode::Subtract:
        out << "Subtract";
        return position + 1;
    case Opcode::Multiply:
        out << "Multiply";
        return position + 1;
    case Opcode::Divide:
        out << "Divide";
        return position + 1;
    case Opcode::Exponent:
        out << "Exponent";
        return position + 1;
    case Opcode::Modulo:
        out << "Modulo";
        return position + 1;
    case Opcode::Equal:
        out << "Equal";
        return position + 1;
    case Opcode::NotEqual:
        out << "NotEqual";
        return position + 1;
    case Opcode::LessThan:
        out << "LessThan";
        return position + 1;
    case Opcode::GreaterThan:
        out << "GreaterThan";
        return position + 1;
    case Opcode::LessThanOrEqual:
        out << "LessThanOrEqual";
        return position + 1;
    case Opcode::GreaterThanOrEqual:
        out << "GreaterThanOrEqual";
        return position + 1;
    case Opcode::GetEnumerator:
        out << "GetEnumerator";
        return position + 1;
    case Opcode::Subscript:
        out << "Subscript";
        return position + 1;
    case Opcode::SetSubscript:
        out << "SetSubscript";
        return position + 1;
    case Opcode::True:
        out << "True";
        return position + 1;
    case Opcode::False:
        out << "False";
        return position + 1;
    case Opcode::Show:
        out << "Show";
        return position + 1;
    case Opcode::Call:
        return disassembleCall(out, "Call", position);
    case Opcode::Empty:
        out << "Empty";
        return position + 1;
    case Opcode::GetIt:
        out << "GetIt";
        return position + 1;
    case Opcode::SetIt:
        out << "SetIt";
        return position + 1;
    }
}

struct BytecodePrinter {
    int depth;

    void print(std::ostream &out, const Bytecode &bytecode) const {
        auto indent = std::string(depth * 2, ' ');
        for (size_t i = 0; i < bytecode.constants().size(); i++) {
            const auto &constant = bytecode.constants()[i];
            out << indent << "[" << i << "] (" << constant.typeName() << ") " << constant
                << std::endl;
            if (const auto &function = constant.as<Function>()) {
                BytecodePrinter printer{depth + 1};
                printer.print(out, *function->bytecode());
            }
        }

        auto position = bytecode.code().begin();
        Optional<Location> previousLocation;
        while (position < bytecode.code().end()) {
            out << indent << bytecode.decodePosition(position);

            auto location = bytecode.location(position);
            if (!previousLocation.has_value() || previousLocation.value() != location) {
                out << std::setfill(' ') << std::setw(8) << std::right
                    << bytecode.location(position) << " ";
            } else {
                out << std::setfill(' ') << std::setw(8) << std::right << "|"
                    << " ";
            }
            previousLocation = location;

            position = bytecode.disassemble(out, position);
            out << std::endl;
        }
    }
};

std::ostream &operator<<(std::ostream &out, const Bytecode &bytecode) {
    BytecodePrinter printer{0};
    printer.print(out, bytecode);
    return out;
}

SIF_NAMESPACE_END
