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

struct NativeCallContext {
    VirtualMachine &vm;
    SourceLocation location;
    Value *arguments;
    std::vector<SourceRange> ranges;

    NativeCallContext(VirtualMachine &vm, SourceLocation location, Value *args,
                      std::vector<SourceRange> ranges = {})
        : vm(vm), location(location), arguments(args), ranges(ranges) {}

    template <class... Args> Error error(std::format_string<Args...> fmt, Args &&...args) const {
        if (ranges.size() > 0) {
            return Error(ranges[0], fmt, std::forward<Args>(args)...);
        } else {
            return Error(location, Format(fmt, std::forward<Args>(args)...));
        }
    }

    template <class... Args>
    Error argumentError(int index, std::format_string<Args...> fmt, Args &&...args) const {
        if (index >= 0 && index < ranges.size()) {
            return Error(ranges[index + 1], fmt, std::forward<Args>(args)...);
        }
        return Error(location, Format(Errors::ArgumentError, index + 1,
                                      Format(fmt, std::forward<Args>(args)...)));
    }
};

class Native : public Object {
  public:
    using Callable = std::function<Result<Value, Error>(const NativeCallContext &)>;

    Native(const Callable &callable);

    const Callable &callable() const;

    std::string typeName() const override;
    std::string description() const override;

  private:
    Callable _callable;
};

SIF_NAMESPACE_END
