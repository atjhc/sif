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
#include "runtime/Value.h"
#include "runtime/Message.h"

#include <vector>
#include <algorithm>
#include <numeric>

CH_RUNTIME_NAMESPACE_BEGIN

class Core;

struct Function {
    virtual ~Function() = default;

    virtual Value valueOf(Core &, const Message &) const = 0;
};

template <auto T>
struct OneArgumentFunction : Function {
    Value valueOf(Core &, const Message &m) const override {
        return Value(T(m.arguments[0]));
    }
};

struct MaxFunction: Function {
    Value valueOf(Core &, const Message &m) const override {
        return *std::max_element(m.arguments.begin(), m.arguments.end());
    }
};

struct MinFunction: Function {
    Value valueOf(Core &, const Message &m) const override {
        return *std::min_element(m.arguments.begin(), m.arguments.end());
    }
};

struct SumFunction: Function {
    Value valueOf(Core &, const Message &m) const override {
        return std::accumulate(m.arguments.begin(), m.arguments.end(), Value(0));
    }
};

struct MeanFunction: Function {
    Value valueOf(Core &, const Message &m) const override {
        return std::accumulate(m.arguments.begin(), m.arguments.end(), Value(0)) / Value(m.arguments.size());
    }
};

struct LengthFunction: Function {
    Value valueOf(Core &, const Message &m) const override {
        return m.arguments[0].asString().length();
    }
};

struct OffsetFunction: Function {
    Value valueOf(Core &, const Message &m) const override {
        auto &v1 = m.arguments[0];
        auto &v2 = m.arguments[1];
        auto result = v2.asString().find(v1.asString());
        return result == std::string::npos ? Value(0) : Value(result);
    }
};

struct ValueFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

struct RandomFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

struct ParamFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

struct ParamsFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

struct ParamCountFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

struct ResultFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

struct TargetFunction: Function {
    Value valueOf(Core &, const Message &m) const override;
};

CH_RUNTIME_NAMESPACE_END
