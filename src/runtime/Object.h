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
#include "runtime/Runtime.h"

#include <unordered_map>

CH_NAMESPACE_BEGIN

class Object {
    std::string _name;
    Owned<ast::Script> _script;
    Strong<Object> _parent;

    Map<std::string, Ref<ast::Handler>> _handlers;
    Map<std::string, Ref<ast::Handler>> _functions;

  public:
    Object(const std::string &n, Owned<ast::Script> &s, const Strong<Object> &parent = nullptr);

    const std::string &name() const { return _name; }
    const Owned<ast::Script> &script() const { return _script; }
    const Strong<Object> &parent() const { return _parent; }

    Optional<Ref<ast::Handler>> handlerFor(const RuntimeMessage &message);
    Optional<Ref<ast::Handler>> functionFor(const RuntimeMessage &message);
};

CH_NAMESPACE_END
