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
#include <sif/runtime/Value.h>

#include <vector>

SIF_NAMESPACE_BEGIN

enum class Opcode : uint8_t {
    Jump,
    JumpIfFalse,
    JumpIfTrue,
    JumpIfAtEnd,
    Repeat,
    Pop,
    Constant,
    OpenRange,
    ClosedRange,
    List,
    UnpackList,
    Dictionary,
    Short,
    Negate,
    Not,
    Increment,
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
    Subscript,
    SetSubscript,
    Enumerate,
    Return,
    True,
    False,
    SetGlobal,
    GetGlobal,
    SetLocal,
    GetLocal,
    SetCapture,
    GetCapture,
    GetEnumerator,
    Show,
    Call,
    Empty,
    GetIt,
    SetIt,
    PushJump,
    PopJump,
    ToString,
};

class Bytecode {
  public:
    using Iterator = std::vector<Opcode>::const_iterator;

    Bytecode() = default;

    size_t add(SourceLocation location, Opcode opcode);
    size_t add(SourceLocation location, Opcode opcode, uint16_t argument);
    void addRepeat(SourceLocation location, uint16_t argument);
    uint16_t addLocal(std::string local);
    uint16_t addConstant(Value constant);
    void addArgumentRanges(size_t callLocation, const std::vector<SourceRange> &ranges);

    void patchRelativeJump(size_t location);
    void patchRelativeJumpTo(size_t location, size_t target);
    void patchAbsoluteJump(size_t location);

    const std::string &name() const;
    const std::vector<Opcode> &code() const;
    const std::vector<std::string> &locals() const;
    const std::vector<Value> &constants() const;
    std::vector<Value> &constants();

    std::vector<SourceRange> argumentRanges(size_t callLocation) const;
    SourceLocation location(Iterator it) const;

    void printWithoutSourceLocations(std::ostream &out) const;

    friend std::ostream &operator<<(std::ostream &out, const Bytecode &bytecode);

  private:
    friend class VirtualMachine;
    friend struct BytecodePrinter;

    std::string decodePosition(Iterator position) const;

    Iterator disassembleConstant(std::ostream &, const std::string &, Iterator) const;
    Iterator disassembleDictionary(std::ostream &out, Iterator position) const;
    Iterator disassembleList(std::ostream &, Iterator) const;
    Iterator disassembleUnpackList(std::ostream &, Iterator) const;
    Iterator disassembleJump(std::ostream &, const std::string &name, Iterator) const;
    Iterator disassembleRepeat(std::ostream &, const std::string &name, Iterator) const;
    Iterator disassembleShort(std::ostream &out, Iterator position) const;
    Iterator disassembleCall(std::ostream &out, const std::string &name, Iterator position) const;
    Iterator disassembleLocal(std::ostream &out, const std::string &name, Iterator position) const;
    Iterator disassemble(std::ostream &, Iterator) const;

    std::string _name;
    std::vector<Opcode> _code;
    std::vector<Value> _constants;
    std::vector<std::string> _locals;
    std::vector<SourceLocation> _locations;
    Mapping<size_t, std::vector<SourceRange>> _argumentRanges;
};

SIF_NAMESPACE_END
