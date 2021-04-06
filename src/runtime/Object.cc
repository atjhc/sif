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
#include "runtime/Runtime.h"
#include "runtime/Property.h"

CH_RUNTIME_NAMESPACE_BEGIN

Object::Object(const std::string &n, Owned<ast::Script> &s, const Strong<Object> &parent)
    : _name(n), _script(std::move(s)), _parent(parent) {

    for (auto &handler : _script->handlers) {
        auto &name = handler->messageKey->name;

        auto map = &_handlers;
        if (handler->kind == ast::Handler::FunctionKind) {
            map = &_functions;
        }

        auto i = map->find(name);
        if (i != map->end()) {
            throw RuntimeError("redefinition of handler " + name, handler->location);
        }

        map->insert({lowercase(name), *handler});
    }
}

Optional<Ref<ast::Handler>> Object::handlerFor(const RuntimeMessage &message) {
    auto i = _handlers.find(message.name);
    if (i == _handlers.end()) {
        return std::nullopt;
    }

    return std::make_optional(i->second);
}

Optional<Ref<ast::Handler>> Object::functionFor(const RuntimeMessage &message) {
    auto i = _functions.find(message.name);
    if (i == _functions.end()) {
        return std::nullopt;
    }

    return std::make_optional(i->second);
}

Value Object::valueForProperty(const Property &p) const {
    if (p.name == "name") {
        return Value(_name);
    }
    throw RuntimeError("unrecognized property");
}

void Object::setValueForProperty(const Value &v, const Property &p) {
    throw RuntimeError("unable to set property");
}

CH_RUNTIME_NAMESPACE_END
