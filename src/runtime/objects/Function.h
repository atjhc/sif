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

#pragma once

#include "Common.h"
#include "compiler/Bytecode.h"
#include "compiler/Signature.h"
#include "runtime/Object.h"

SIF_NAMESPACE_BEGIN

class Function : public Object {
  public:
    struct Capture {
        int index;
        bool isLocal;
    };

    Function(const Signature &signature, const Strong<Bytecode> &bytecode, const std::vector<Capture> &captures = {});

    const Strong<Bytecode> &bytecode() const;
    const std::vector<Capture> &captures() const;
    
    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;

  private:
    Signature _signature;
    Strong<Bytecode> _bytecode;
    std::vector<Capture> _captures;
};

SIF_NAMESPACE_END
