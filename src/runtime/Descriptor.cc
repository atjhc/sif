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

#include "runtime/Descriptor.h"
#include "runtime/Interpreter.h"
#include "Utilities.h"

CH_RUNTIME_NAMESPACE_BEGIN

Descriptor::Descriptor(Interpreter &interpreter, const ast::Descriptor &descriptor) : names(descriptor) {
    if (descriptor.value) {
        value = interpreter.evaluate(*descriptor.value);
    }
}

Descriptor::Descriptor(const Names &names, const Optional<Value> &value)
    : names(names), value(value) {}

std::string Descriptor::description() const {
    return String(names.description(), " ", Quoted(value.value().asString()));
}

CH_RUNTIME_NAMESPACE_END
