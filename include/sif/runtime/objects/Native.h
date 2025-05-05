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

#include <sif/Common.h>
#include <sif/compiler/Signature.h>
#include <sif/runtime/Object.h>
#include <sif/runtime/Value.h>
#include <sif/runtime/VirtualMachine.h>

SIF_NAMESPACE_BEGIN

class Native : public Object {
  public:
    using Callable = std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)>;

    Native(const Callable &callable);

    const Callable &callable() const;

    std::string typeName() const override;
    std::string description() const override;

  private:
    Callable _callable;
};

SIF_NAMESPACE_END
