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
#include "ast/Script.h"
#include "ast/Handler.h"

CH_RUNTIME_NAMESPACE_BEGIN

struct Message;
struct Property;

class Object {
    std::string _name;
    Owned<ast::Script> _script;
    Strong<Object> _parent;

    Map<std::string, Ref<ast::Handler>> _handlers;
    Map<std::string, Ref<ast::Handler>> _functions;

  public:
    Object(const std::string &n, Owned<ast::Script> &s, const Strong<Object> &parent = nullptr);
    virtual ~Object() = default;

    const std::string &name() const { return _name; }
    const Owned<ast::Script> &script() const { return _script; }
    const Strong<Object> &parent() const { return _parent; }

    Optional<Ref<ast::Handler>> handlerFor(const Message &message);
    Optional<Ref<ast::Handler>> functionFor(const Message &message);

    virtual Value valueForProperty(const Property &p) const;
    virtual void setValueForProperty(const Value &v, const Property &p);
};

CH_RUNTIME_NAMESPACE_END
