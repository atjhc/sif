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

#include "runtime/Folder.h"
#include "runtime/Property.h"

#include <sstream>
#include <filesystem>

CH_RUNTIME_NAMESPACE_BEGIN

namespace fs = std::filesystem;

Strong<Folder> Folder::Make(const std::string &path) {
    return Strong<Folder>(new Folder(path));
}

Folder::Folder(const std::string &path) : Path(path) {}

Optional<Value> Folder::valueForProperty(const Property &p) const {
    if (p.is("contents")) {
        return asString().value();
    }
    return Path::valueForProperty(p);
}

bool Folder::setValueForProperty(const Value &v, const Property &p) {
    return Object::setValueForProperty(v, p);
}

Optional<std::string> Folder::asString() const {
    std::ostringstream ss;
    auto it = fs::directory_iterator(_path);
    while (it != fs::end(it)) {
        ss << it->path().string();
        it++;
        if (it != fs::end(it)) {
            ss << '\n';
        }
    }
    return ss.str();
}

bool Folder::exists() const {
    return fs::is_directory(_path);
}

CH_RUNTIME_NAMESPACE_END
