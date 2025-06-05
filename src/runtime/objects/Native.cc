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

#include "sif/runtime/objects/Native.h"

SIF_NAMESPACE_BEGIN

Native::Native(const Native::Callable &callable) : _callable(callable) {}

const Native::Callable &Native::callable() const { return _callable; }

std::string Native::typeName() const { return "function"; }

std::string Native::description() const { return "<native function>"; }

SIF_NAMESPACE_END
