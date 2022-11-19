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
#include "Error.h"

SIF_NAMESPACE_BEGIN

class Reader {
  public:
    virtual ~Reader() = default;

    virtual bool readable() const = 0;
    virtual Optional<Error> read(int scopeDepth) = 0;
    virtual const std::string &contents() const = 0;
};

class StringReader : public Reader {
  public:
    StringReader(const std::string &contents);

    bool readable() const override;
    Optional<Error> read(int depth) override;
    const std::string &contents() const override;

  private:
    std::string _contents;
};

class FileReader : public Reader {
  public:
    FileReader(const std::string &path);

    bool readable() const override;
    Optional<Error> read(int scopeDepth) override;
    const std::string &contents() const override;

  private:
    std::string _path;
    std::string _contents;
};

SIF_NAMESPACE_END
