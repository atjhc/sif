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
#include <sif/runtime/objects/Native.h>

SIF_NAMESPACE_BEGIN

class Module {
  public:
    virtual std::vector<Signature> signatures() const = 0;
    virtual Mapping<std::string, Value> values() const = 0;
};

class ModuleProvider {
  public:
    virtual Result<Strong<Module>, Error> module(const std::string &name) = 0;
};

class UserModule : public Module {
  public:
    UserModule(const std::string &name, const std::vector<Signature> &signatures,
               const Mapping<std::string, Value> &values);
    virtual ~UserModule() = default;

    const std::string &name() const;

    std::vector<Signature> signatures() const override;
    Mapping<std::string, Value> values() const override;

  private:
    std::string _name;
    std::vector<Signature> _signatures;
    Mapping<std::string, Value> _values;
};

SIF_NAMESPACE_END
