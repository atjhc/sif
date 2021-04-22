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
#include "ast/Identifier.h"

CH_RUNTIME_NAMESPACE_BEGIN

Container::Container(const Owned<ast::Expression> &e) {
    auto target = e.get();
    while (target) {
        if (const auto identifier = dynamic_cast<ast::Identifier *>(target)) {
            name = identifier->name;
            target = nullptr;
        } else if (const auto chunkExpression = dynamic_cast<ast::ChunkExpression *>(target)) {
            chunkList.push_back(*chunkExpression->chunk);
            target = chunkExpression->expression.get();
        } else {
            throw RuntimeError("unexpected expression", target->location);
        }
    }
}

CH_RUNTIME_NAMESPACE_END
