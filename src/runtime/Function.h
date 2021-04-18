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
#include "runtime/Message.h"
#include "runtime/Value.h"

#include <algorithm>
#include <numeric>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

class Interpreter;

struct Function {
    virtual ~Function() = default;
    virtual Value valueOf(Interpreter &, const Message &) const = 0;

  protected:
    void expectArgumentCount(const Message &m, int count) const;
    void expectArguments(const Message &m) const;
    void expectNumberAt(const Message &m, int index) const;
    void expectAllNumbers(const Message &m) const;
};

template <double F(double)> struct OneArgumentFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override {
        expectArgumentCount(m, 1);
        expectNumberAt(m, 0);

        return Value(F(m.arguments[0].asFloat()));
    }
};

struct MaxFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct MinFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct SumFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct MeanFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct LengthFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct OffsetFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct SecondsFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct ValueFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct RandomFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct ParamFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct ParamsFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct ParamCountFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct ResultFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

struct TargetFunction : Function {
    Value valueOf(Interpreter &, const Message &m) const override;
};

CH_RUNTIME_NAMESPACE_END
