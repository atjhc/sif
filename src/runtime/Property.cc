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

#include "runtime/Property.h"
#include "Utilities.h"

#include "ast/Identifier.h"

CH_RUNTIME_NAMESPACE_BEGIN

Property::Property(const std::string &name) {
    names.push_back(name);
}

Property::Property(const std::string &name1, const std::string &name2) {
    names.push_back(name2);
    names.push_back(name1);
}

Property::Property(const std::vector<std::string> &names)
    : names(names) {}

Property::Property(const ast::Property &p) {
    for (auto &identifier : p.identifiers->identifiers) {
        names.push_back(lowercase(identifier->name));
    }
}

Property::Property(const ast::FunctionCall &fn) {
    names.push_back(lowercase(fn.name->name));
}

std::string Property::description() const {
    return Join(names, " ");
}

bool Property::operator==(const Property &property) const {
    return names == property.names;
}

bool Property::is(const std::string &n) const {
    return names.size() == 1 && names[0] == lowercase(n);
}

bool Property::is(const std::string &n1, const std::string &n2) const {
    return names.size() == 2 && names[0] == lowercase(n1) && names[1] == lowercase(n2);
}

CH_RUNTIME_NAMESPACE_END
