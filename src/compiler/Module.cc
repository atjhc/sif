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

#include "sif/compiler/Module.h"

SIF_NAMESPACE_BEGIN

UserModule::UserModule(const std::string &name, const std::vector<Signature> &signatures,
                       const Mapping<std::string, Value> &values)
    : _name(name), _signatures(signatures), _values(values) {}

const std::string &UserModule::name() const { return _name; }

std::vector<Signature> UserModule::signatures() const { return _signatures; }
Mapping<std::string, Value> UserModule::values() const { return _values; }

SIF_NAMESPACE_END
