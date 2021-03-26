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

#include "Common.h"
#include "ast/Script.h"

CH_NAMESPACE_BEGIN

CH_DECL_CLASS_REF(Object)
class Object {
    std::string _name;
    std::unique_ptr<ast::Script> _script;

  public:
    Object(const std::string &n, std::unique_ptr<ast::Script> &s)
        : _name(n), _script(std::move(s)) {}

    const std::string &name() const { return _name; }

    const std::unique_ptr<ast::Script> &script() const { return _script; }
};

CH_NAMESPACE_END
