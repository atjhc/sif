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

#include "runtime/objects/Function.h"

SIF_NAMESPACE_BEGIN

Function::Function(const Signature &signature, const Strong<Bytecode> &bytecode,
                   const std::vector<Capture> &captures)
    : _signature(signature), _bytecode(bytecode), _captures(captures) {}

const Strong<Bytecode> &Function::bytecode() const { return _bytecode; }
const std::vector<Function::Capture> &Function::captures() const { return _captures; }

std::string Function::typeName() const { return "function"; }

std::string Function::description() const { return _signature.name(); }

bool Function::equals(Strong<Object> object) const { return this == object.get(); }

SIF_NAMESPACE_END
