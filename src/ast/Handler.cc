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

#include "Handler.h"

CH_AST_NAMESPACE_BEGIN

void Handler::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
    switch (kind) {
    case HandlerKind:
        out << context.indentString() << "on ";
        break;
    case FunctionKind:
        out << context.indentString() << "function ";
        break;
    }

    messageKey->prettyPrint(out, context);
    if (arguments) {
        out << " ";
        arguments->prettyPrint(out, context);
    }
    out << std::endl;
    if (statements) {
        statements->prettyPrint(out, context);
    }
    out << context.indentString() << "end ";
    messageKey->prettyPrint(out, context);
    out << std::endl;
}

void IdentifierList::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
    auto i = identifiers.begin();
    while (i != identifiers.end()) {
        (*i)->prettyPrint(out, context);

        i++;
        if (i != identifiers.end()) {
            out << ", ";
        }
    }
}

CH_AST_NAMESPACE_END
