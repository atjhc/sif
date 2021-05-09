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

#include "runtime/Names.h"
#include "Utilities.h"

#include "ast/Identifier.h"

CH_RUNTIME_NAMESPACE_BEGIN

Names::Names(const std::string &name) {
    names.push_back(name);
}

Names::Names(const std::string &name1, const std::string &name2) {
    names.push_back(name2);
    names.push_back(name1);
}

Names::Names(const std::vector<std::string> &names)
    : names(names) {}

Names::Names(const ast::Descriptor &descriptor) {
    for (auto &identifier : descriptor.identifiers->identifiers) {
        names.push_back(lowercase(identifier->name));
    }
}

Names::Names(const ast::Property &p) {
    for (auto &identifier : p.identifiers->identifiers) {
        names.push_back(lowercase(identifier->name));
    }
}

Names::Names(const ast::FunctionCall &fn) {
    names.push_back(lowercase(fn.name->name));
}

std::string Names::description() const {
    return Join(names, " ");
}

bool Names::operator==(const Names &property) const {
    return names == property.names;
}

bool Names::is(const std::string &n) const {
    return names.size() == 1 && names[0] == lowercase(n);
}

bool Names::is(const std::string &n1, const std::string &n2) const {
    return names.size() == 2 && names[0] == lowercase(n1) && names[1] == lowercase(n2);
}

CH_RUNTIME_NAMESPACE_END
