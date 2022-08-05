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
#include "runtime/Enumerable.h"
#include "runtime/Object.h"
#include "runtime/Value.h"
#include "runtime/objects/Range.h"

#include <string>

SIF_NAMESPACE_BEGIN

class String : public Object, public Enumerable {
  public:
    String(const std::string &string);

    const std::string &string() const;

    Value operator[](const Range &range) const;

    std::string typeName() const override;
    std::string description() const override;
    bool equals(Strong<Object>) const override;

    // Enumerable
    int64_t length() const override;
    Value operator[](int64_t) const override;

  private:
    std::string _string;
};

SIF_NAMESPACE_END
