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

#include "runtime/objects/File.h"
#include "runtime/Names.h"

#include <fstream>
#include <filesystem>

CH_RUNTIME_NAMESPACE_BEGIN

namespace fs = std::filesystem;

Strong<File> File::Make(const std::string &path) {
    return Strong<File>(new File(path));
}

File::File(const std::string &path) : FileSystemItem(path) {}

Optional<Value> File::valueForProperty(const Names &p) const {
    if (p.is("contents")) {
        return asString().value();
    }
    if (p.is("size")) {
        return fs::file_size(_path);
    }
    return FileSystemItem::valueForProperty(p);
}

bool File::setValueForProperty(const Value &v, const Names &p) {
    if (p.is("contents")) {
        setString(v.asString());
        return true;
    }
    return Object::setValueForProperty(v, p);
}

Optional<std::string> File::asString() const {
    std::ifstream file(_path);
    if (file) {
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    return Value();
}

void File::setString(const std::string &contents) const {
    std::ofstream file(_path);
    if (!file) {
        throw RuntimeError(String("could not write to file ", Quoted(_path)));
    }
    file << contents;
}

bool File::exists() const {
    return fs::is_regular_file(_path);
}

CH_RUNTIME_NAMESPACE_END
