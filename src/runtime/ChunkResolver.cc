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

#include "runtime/ChunkResolver.h"
#include "runtime/Interpreter.h"

CH_RUNTIME_NAMESPACE_BEGIN

chunk ChunkResolver::resolve(const ast::Chunk &chunk) {
    return chunk.accept(*this);
}

chunk ChunkResolver::resolve(const std::vector<Ref<ast::Chunk>> &chunkList, Interpreter &interpreter, std::string &source) {
    chunk c = chunk(chunk::character, source);
    for (const auto &node : reversed(chunkList)) {
        auto resolver = ChunkResolver(interpreter, c);
        c = resolver.resolve(node.get());
    }
    return c;
}

static chunk::type chunk_type(ast::Chunk::Type t) {
    switch (t) {
    case ast::Chunk::Char:
        return chunk::character;
    case ast::Chunk::Word:
        return chunk::word;
    case ast::Chunk::Item:
        return chunk::item;
    case ast::Chunk::Line:
        return chunk::line;
    }
}

chunk ChunkResolver::_baseChunk() const {
    return chunk(chunk::character, _range, _itemDelimiter());
}

unsigned char ChunkResolver::_itemDelimiter() const {
    auto property = Names("itemdelimiter");
    std::string delimiter = _interpreter.valueForProperty(property).value_or(",");
    return delimiter[0];
}

chunk ChunkResolver::visit(const ast::RangeChunk &c) {
    auto startValue = _interpreter.evaluate(*c.start);
    if (!startValue.isInteger()) {
        throw RuntimeError(String("expected integer here, got ", Quoted(startValue.asString())), c.start->location);
    }

    if (c.end) {
        auto endValue = _interpreter.evaluate(*c.end);
        if (!endValue.isInteger()) {
            throw RuntimeError(String("expected integer here, got ", Quoted(endValue.asString())), c.end->location);
        }

        return range_chunk(chunk_type(c.type), 
                           startValue.asInteger() - 1,
                           endValue.asInteger() - 1, 
                           _baseChunk(),
                           _itemDelimiter());
    } 
    
    return index_chunk(chunk_type(c.type),
                 startValue.asInteger() - 1,
                 _baseChunk(),
                 _itemDelimiter());
}

chunk ChunkResolver::visit(const ast::AnyChunk &c) {
    auto random = [this](int count) { 
        return _interpreter.random()() * count;
    };
    return random_chunk(chunk_type(c.type),
                        random,
                        _baseChunk(),
                        _itemDelimiter()); 
}

chunk ChunkResolver::visit(const ast::LastChunk &c) {
    return last_chunk(chunk_type(c.type), _baseChunk(), _itemDelimiter());
}

chunk ChunkResolver::visit(const ast::MiddleChunk &c) {
    return middle_chunk(chunk_type(c.type), _baseChunk(), _itemDelimiter());
}

CH_RUNTIME_NAMESPACE_END
