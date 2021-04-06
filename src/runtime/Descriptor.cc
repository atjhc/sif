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
#include "runtime/Runtime.h"

CH_RUNTIME_NAMESPACE_BEGIN

Descriptor::Descriptor(Runtime &r, const ast::Descriptor &d) {
    names.push_back(d.name->name);

    auto v = d.value.get();
    while (v) {
        if (const auto desc = dynamic_cast<ast::Descriptor *>(v)) {
            names.push_back(desc->name->name);
            v = desc->value.get();
        } else {
            value = v->evaluate(r);
            v = nullptr;
        }
    }
}

CH_RUNTIME_NAMESPACE_END
