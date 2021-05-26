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
#include "runtime/Value.h"

#include <vector>

CH_NAMESPACE_BEGIN

enum class Opcode : uint8_t {
    Jump,
    JumpIfFalse,
    JumpIfTrue,
    Repeat,
    Pop,
    Constant,
    List,
    Short,
    Negate,
    Not,
    Add,
    Subtract,
    Multiply,
    Divide,
    Exponent,
    Modulo,
    Equal,
    NotEqual,
    LessThan,
    GreaterThan,
    LessThanOrEqual,
    GreaterThanOrEqual,
    Return,
    True,
    False,
    And,
    Or,
    SetVariable,
    GetVariable,
    Show,
    Call
};

class Bytecode {
public:
    using Iterator = std::vector<Opcode>::const_iterator;

    Bytecode() = default;

    size_t add(Location location, Opcode opcode);
    size_t add(Location location, Opcode opcode, uint16_t argument);
    void addRepeat(Location location, uint16_t argument);
    uint16_t addConstant(Value constant);
    void patchJump(size_t location);

    const std::string &name() const;
    const std::vector<Opcode> &code() const;
    const std::vector<Value> &constants() const;
    
    Location location(Iterator it) const;

    friend std::ostream &operator<<(std::ostream &out, const Bytecode &bytecode);

private:
    Iterator disassembleConstant(std::ostream &, const std::string &, Iterator) const;
    Iterator disassembleList(std::ostream &, Iterator) const;
    Iterator disassembleJump(std::ostream &, const std::string &name, Iterator) const;
    Iterator disassembleShort(std::ostream &out, Iterator position) const;
    Iterator disassemble(std::ostream &, Iterator) const;

    std::string _name;
    std::vector<Opcode> _code;
    std::vector<Value> _constants;
    std::vector<Location> _locations;
};

CH_NAMESPACE_END
