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
#include "compiler/Module.h"

#include <functional>
#include <random>

SIF_NAMESPACE_BEGIN

struct CoreConfig {
    std::ostream &out = std::cout;
    std::istream &in = std::cin;
    std::ostream &err = std::cerr;
    std::mt19937_64 engine = std::mt19937_64(std::random_device()());
    std::function<Integer(Integer)> randomInteger = [this](Integer max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(engine);
    };
};

class Core : public Module {
  public:
    Core(const CoreConfig &config = CoreConfig());

    std::vector<Signature> signatures() const override;
    Mapping<std::string, Strong<Native>> functions() const override;

  private:
    Mapping<Signature, Strong<Native>, Signature::Hash> _natives;
    CoreConfig _config;
};

SIF_NAMESPACE_END
