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

#include "ast/Handler.h"
#include "ast/Identifier.h"

CH_AST_NAMESPACE_BEGIN

Handler::Handler(Kind _kind, Owned<Identifier> &mk, Owned<IdentifierList> &args,
        Owned<StatementList> &sl)
    : kind(_kind), messageKey(std::move(mk)), arguments(std::move(args)), statements(std::move(sl)) {}

CH_AST_NAMESPACE_END
