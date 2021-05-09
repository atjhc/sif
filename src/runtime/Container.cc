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

#include "runtime/Container.h"
#include "runtime/Error.h"
#include "runtime/Interpreter.h"
#include "runtime/Descriptor.h"
#include "ast/Descriptor.h"

CH_RUNTIME_NAMESPACE_BEGIN

Container::Container(Interpreter &interpreter, const Owned<ast::Expression> &expression) {
    auto target = expression.get();
    while (target) {
        if (const auto d = dynamic_cast<ast::Descriptor *>(target)) {
            descriptor = MakeOwned<Descriptor>(interpreter, *d);
            target = nullptr;
        } else if (const auto e = dynamic_cast<ast::ChunkExpression *>(target)) {
            chunkList.push_back(*e->chunk);
            target = e->expression.get();
        } else {
            throw RuntimeError("unexpected expression", target->location);
        }
    }
}

CH_RUNTIME_NAMESPACE_END
