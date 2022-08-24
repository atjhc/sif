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

#include "Reader.h"

#include <iostream>

SIF_NAMESPACE_BEGIN

StringReader::StringReader(const std::string &contents) : _contents(contents) {}

bool StringReader::readable() const { return false; }

Optional<ReadError> StringReader::read(int depth) { return None; }

const std::string &StringReader::contents() const { return _contents; }

SIF_NAMESPACE_END
