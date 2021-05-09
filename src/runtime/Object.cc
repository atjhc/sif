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

#include "runtime/Object.h"
#include "runtime/Interpreter.h"
#include "runtime/Names.h"

CH_RUNTIME_NAMESPACE_BEGIN

Strong<Object> Object::Make(const std::string &name, const std::string &source,
                            const Strong<Object> &parent) {
    if (source.empty()) {
        return Strong<Object>(new Object(name, source, nullptr, parent));
    }

    ParserConfig config;
    config.fileName = name;

    Parser parser(config);
    Owned<ast::Program> program;
    if ((program = parser.parseProgram(source)) == nullptr) {
        return nullptr;
    }

    return Strong<Object>(new Object(name, source, std::move(program), parent));
}

Object::Object(const std::string &name, const std::string &source, Owned<ast::Program> program,
               const Strong<Object> &parent)
    : _source(source), _program(std::move(program)), _parent(parent), _name(name) {

    if (_program) {
        for (auto &handler : _program->handlers) {
            auto &name = handler->messageKey->name;

            auto map = &_handlers;
            if (handler->kind == ast::Handler::FunctionKind) {
                map = &_functions;
            }

            // TODO: catch handler redifinition errors in the parser.
            if (map->find(name) != map->end()) {
                throw RuntimeError(String("invalid redefinition of handler ", Quoted(name)),
                                   handler->location);
            }
            map->insert({lowercase(name), *handler});
        }
    }
}

Optional<Ref<ast::Handler>> Object::handlerFor(const Message &message) {
    auto i = _handlers.find(message.name);
    if (i == _handlers.end()) {
        return Empty;
    }
    return i->second;
}

Optional<Ref<ast::Handler>> Object::functionFor(const Message &message) {
    auto i = _functions.find(message.name);
    if (i == _functions.end()) {
        return Empty;
    }
    return i->second;
}

Optional<Value> Object::valueForProperty(const Names &p) const {
    if (p.is("name")) {
        return Value(_name);
    }
    return Empty;
}

bool Object::setValueForProperty(const Value &v, const Names &p) { return false; }

Optional<std::string> Object::asString() const {
    return _name;
}

bool Object::exists() const {
    return true;
}

CH_RUNTIME_NAMESPACE_END
