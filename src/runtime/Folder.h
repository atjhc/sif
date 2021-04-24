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
#include "runtime/Path.h"

CH_RUNTIME_NAMESPACE_BEGIN

class Folder : public Path {
  public:
    static Strong<Folder> Make(const std::string &path);

    ~Folder() = default;

    Optional<Value> valueForProperty(const Property &p) const override;
    bool setValueForProperty(const Value &v, const Property &p) override;

    bool exists() const override;
    Optional<std::string> asString() const override;

  private:
    Folder(const std::string &path);
};

CH_RUNTIME_NAMESPACE_END
