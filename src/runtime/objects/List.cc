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

#include "runtime/objects/List.h"

CH_NAMESPACE_BEGIN

List::List(const std::vector<Value> &values) : _values(values) {}

const std::vector<Value> &List::values() const {
    return _values;
}
    
std::string List::typeName() const {
    return "list";
}

std::string List::description() const {
    return Concat("[", Join(_values, ", "), "]");
}

bool List::equals(Strong<Object> object) const {
    if (const auto &list = std::dynamic_pointer_cast<List>(object)) {
        return _values == list->_values;
    }
    return false;
}

CH_NAMESPACE_END
