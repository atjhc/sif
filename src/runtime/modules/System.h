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

SIF_NAMESPACE_BEGIN

class System : public Module {
  public:
    System();

    void setArguments(char **argv);
    void setEnvironment(char **envp);

    void setSystemName(const std::string &);
    void setSystemVersion(const std::string &);

    std::vector<Signature> signatures() const override;
    Mapping<std::string, Value> values() const override;

  private:
    static void _files(Mapping<Signature, Strong<Native>, Signature::Hash> &natives);

    Mapping<Signature, Strong<Native>, Signature::Hash> _natives;

    std::vector<std::string> _arguments;
    Mapping<std::string, std::string> _environment;
    std::string _systemName;
    std::string _systemVersion;
};

SIF_NAMESPACE_END
