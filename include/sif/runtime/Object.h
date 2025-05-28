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

#include <functional>

SIF_NAMESPACE_BEGIN

class Object {
  public:
    virtual ~Object() = default;

    virtual std::string typeName() const = 0;
    virtual bool equals(Strong<Object>) const;
    virtual size_t hash() const;

    virtual std::string toString() const;
    virtual std::string description() const = 0;
    virtual std::string description(Set<const Object *> &visited) const;
    virtual std::string debugDescription() const;

    virtual void trace(const std::function<void(Strong<Object> &)> &visitor) {}
    bool visited = false;
};

std::ostream &operator<<(std::ostream &out, const Strong<Object> &object);

SIF_NAMESPACE_END
