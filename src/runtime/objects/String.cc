
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

#include "runtime/objects/String.h"

CH_NAMESPACE_BEGIN

String::String(const std::string &string) : _string(string) {}

const std::string &String::string() const {
    return _string;
}
    
std::string String::typeName() const {
    return "string";
}

std::string String::description() const {
    return _string;
}

bool String::equals(Strong<Object> object) const {
    if (const auto &string = std::dynamic_pointer_cast<String>(object)) {
        return _string == string->_string;
    }
    return false;
}

CH_NAMESPACE_END
