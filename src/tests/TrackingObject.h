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

#include <sif/runtime/Object.h>

SIF_NAMESPACE_BEGIN

class TrackingObject : public Object {
  public:
    TrackingObject();
    ~TrackingObject();

    std::string typeName() const override;
    std::string description() const override;

    static int count;
};

SIF_NAMESPACE_END