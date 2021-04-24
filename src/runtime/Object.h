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
#include "ast/Handler.h"
#include "ast/Program.h"
#include "runtime/Value.h"

CH_RUNTIME_NAMESPACE_BEGIN

struct Message;
struct Property;

class Object {
  public:
    static Strong<Object> Make(const std::string &name, const std::string &source = "",
                               const Strong<Object> &parent = nullptr);

    virtual ~Object() = default;

    const std::string &name() const { return _name; }
    const std::string &source() const { return _source; }
    const Strong<Object> &parent() const { return _parent; }

    Optional<Ref<ast::Handler>> handlerFor(const Message &message);
    Optional<Ref<ast::Handler>> functionFor(const Message &message);

    virtual Optional<Value> valueForProperty(const Property &p) const;
    virtual bool setValueForProperty(const Value &v, const Property &p);

    virtual bool exists() const;
    virtual Optional<std::string> asString() const;

  private:
    std::string _source;
    Owned<ast::Program> _program;
    Strong<Object> _parent;

    Map<std::string, Ref<ast::Handler>> _handlers;
    Map<std::string, Ref<ast::Handler>> _functions;
  
  protected:
    std::string _name;

    Object(const std::string &name, const std::string &source, Owned<ast::Program> program,
           const Strong<Object> &parent);
};

CH_RUNTIME_NAMESPACE_END
