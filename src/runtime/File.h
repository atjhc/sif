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
#include "runtime/FSObject.h"

CH_RUNTIME_NAMESPACE_BEGIN

class File : public FSObject {
  public:
    static Strong<File> Make(const std::string &path);

    ~File() = default;

    Optional<Value> valueForProperty(const Property &p) const override;
    bool setValueForProperty(const Value &v, const Property &p) override;

    bool exists() const override;
    Optional<std::string> asString() const override;

  private:
    File(const std::string &path);
};

CH_RUNTIME_NAMESPACE_END
