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
#include "runtime/Object.h"
#include "runtime/Value.h"

#include <string>

SIF_NAMESPACE_BEGIN

class Range : public Object {
  public:
    Range(Optional<int64_t> start, Optional<int64_t> end, bool closed);

    Optional<int64_t> start() const;
    Optional<int64_t> end() const;
    bool closed() const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;

  private:
    Optional<int64_t> _start;
    Optional<int64_t> _end;
    bool _closed;
};

SIF_NAMESPACE_END
