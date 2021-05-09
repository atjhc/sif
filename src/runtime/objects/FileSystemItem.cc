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

#include "runtime/objects/FileSystemItem.h"
#include "runtime/Names.h"

#include <fstream>
#include <filesystem>

CH_RUNTIME_NAMESPACE_BEGIN

namespace fs = std::filesystem;

FileSystemItem::FileSystemItem(const std::string &path) : Object(String("path ", path), "", nullptr, nullptr), _path(path) {}

Optional<Value> FileSystemItem::valueForProperty(const Names &p) const {
    if (p.is("path")) {
        return _path;
    }
    if (p.is("name")) {
        return fs::path(_path).filename();
    }
    if (p.is("dirname")) {
        return fs::path(_path).parent_path();
    }
    if (p.is("extension")) {
        return fs::path(_path).extension();
    }
    if (p.is("stem")) {
        return fs::path(_path).stem();
    }
    return Object::valueForProperty(p);
}

bool FileSystemItem::setValueForProperty(const Value &v, const Names &p) {
    return Object::setValueForProperty(v, p);
}

Optional<std::string> FileSystemItem::asString() const {
    return _path;
}

bool FileSystemItem::exists() const {
    return false;
}

CH_RUNTIME_NAMESPACE_END
