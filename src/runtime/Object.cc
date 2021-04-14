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
#include "runtime/Property.h"

CH_RUNTIME_NAMESPACE_BEGIN

Strong<Object> Object::Make(const std::string &name, const std::string &source, const Strong<Object> &parent) {
    if (source.empty()) {
        return Strong<Object>(new Object(name, parent));
    }

    ParserConfig config;
    config.fileName = name;

    Parser parser(config);
    Owned<ast::Program> result;
    if ((result = parser.parseProgram(source)) == nullptr) {
        return nullptr;
    }

    return Strong<Object>(new Object(name, result, parent));
}

Object::Object(const std::string &n, const Strong<Object> &p)
    : _name(n), _parent(p) {}

Object::Object(const std::string &n, Owned<ast::Program> &s, const Strong<Object> &p)
    : Object(n, p) {

    _program = std::move(s);
    for (auto &handler : _program->handlers) {
        auto &name = handler->messageKey->name;

        auto map = &_handlers;
        if (handler->kind == ast::Handler::FunctionKind) {
            map = &_functions;
        }

        // TODO: catch handler redifinition errors in the parser.
        if (map->find(name) != map->end()) {
            throw RuntimeError("invalid redefinition of handler '" + name + "'", handler->location);
        }
        map->insert({lowercase(name), *handler});
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

Optional<Value> Object::valueForProperty(const Property &p) const {
    if (p.name == "name") {
        return Value(_name);
    }
    return Empty;
}

bool Object::setValueForProperty(const Value &v, const Property &p) {
    return false;
}

CH_RUNTIME_NAMESPACE_END
