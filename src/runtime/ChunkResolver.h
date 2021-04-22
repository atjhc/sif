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
#include "ast/Chunk.h"
#include "runtime/Value.h"
#include "utilities/chunk.h"

#include <string>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

class Interpreter;

class ChunkResolver : ast::Chunk::Visitor<chunk> {
public:

    template <typename T>
    ChunkResolver(Interpreter &interpreter, T &i) 
        : _interpreter(interpreter), _range(i.begin(), i.end()) {}

    static chunk resolve(const std::vector<Ref<ast::Chunk>> &chunkList,
                         Interpreter &interpreter,
                         std::string &source);

    chunk resolve(const ast::Chunk &chunk);

    std::string::iterator begin() { return _range.begin(); }
    std::string::iterator end() { return _range.end(); }
    
    chunk visit(const ast::RangeChunk &) override;
    chunk visit(const ast::AnyChunk &) override;
    chunk visit(const ast::LastChunk &) override;
    chunk visit(const ast::MiddleChunk &) override;

private:
    Interpreter &_interpreter;
    range<std::string> _range;
};

CH_RUNTIME_NAMESPACE_END
