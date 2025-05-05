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

#include <string>

SIF_NAMESPACE_BEGIN

std::string string_from_escaped_string(const std::string &str);

std::string escaped_string_from_string(const std::string &str);

std::string encode_utf8(uint32_t codepoint);

uint32_t decode_utf8(const std::string &utf8);

SIF_NAMESPACE_END
