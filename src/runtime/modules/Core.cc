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

#include "Common.h"
#include "Error.h"
#include "Utilities.h"

#include "runtime/modules/Core.h"
#include "runtime/objects/Dictionary.h"
#include "runtime/objects/List.h"
#include "runtime/objects/Native.h"
#include "runtime/objects/String.h"

#include "utilities/chunk.h"
#include "vendor/utf8.h"

#include <random>

SIF_NAMESPACE_BEGIN

using ModuleMap = std::unordered_map<Signature, Strong<Native>, Signature::Hash>;
static Signature S(const char *signature) { return Signature::Make(signature).value(); }

ModuleMap _coreNatives = []() -> ModuleMap {
    ModuleMap natives;
    natives[S("quit")] = MakeStrong<Native>(
        [](Location location, Value *values) -> Result<Value, RuntimeError> { exit(0); });
    natives[S("quit with {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (!values[0].isInteger()) {
                return Error(RuntimeError(location, "expected an integer"));
            }
            exit(values[0].asInteger());
        });
    natives[S("write {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (const auto &list = values[0].as<List>()) {
                std::cout << Join(list->values(), " ");
            } else {
                std::cout << values[0];
            }
            return Value();
        });
    natives[S("write error {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (const auto &list = values[0].as<List>()) {
                std::cerr << Join(list->values(), " ");
            } else {
                std::cerr << values[0];
            }
            return Value();
        });
    natives[S("print {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (const auto &list = values[0].as<List>()) {
                std::cout << Join(list->values(), " ");
            } else {
                std::cout << values[0];
            }
            std::cout << std::endl;
            return Value();
        });
    natives[S("print error {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (const auto &list = values[0].as<List>()) {
                for (const auto &item : list->values()) {
                    std::cerr << item;
                }
            } else {
                std::cerr << values[0];
            }
            std::cerr << std::endl;
            return Value();
        });
    natives[S("get {}")] = MakeStrong<Native>(
        [](Location location, Value *values) -> Result<Value, RuntimeError> { return values[0]; });
    natives[S("read (a) word")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            std::string input;
            std::cin >> input;
            return input;
        });
    natives[S("read (a) line")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            std::string input;
            std::getline(std::cin, input);
            return input;
        });
    natives[S("read (a) character")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            char input;
            if (std::cin.get(input)) {
                return input;
            } else {
                return Value();
            }
        });
    natives[S("(the) debug description of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            return values[0].debugDescription();
        });
    natives[S("(the) description of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            std::ostringstream ss;
            ss << values[0];
            return ss.str();
        });
    natives[S("(the) hash value of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            return Integer(Value::Hash()(values[0]));
        });
    natives[S("(the) type name of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            return lowercase(values[0].typeName());
        });
    return natives;
}();

ModuleMap _commonNatives = []() -> ModuleMap {
    ModuleMap natives;
    natives[S("(the) size of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            size_t size = 0;
            if (auto list = values[0].as<List>()) {
                size = list->values().size();
            } else if (auto dictionary = values[0].as<Dictionary>()) {
                size = dictionary->values().size();
            } else if (auto string = values[0].as<String>()) {
                size = string->string().size();
            } else if (auto range = values[0].as<Range>()) {
                size = range->size();
            } else {
                return Error(
                    RuntimeError(location, "expected a string, list, dictionary, or range"));
            }
            return static_cast<long>(size);
        });
    auto contains = [](Location location, Value object,
                       Value value) -> Result<Value, RuntimeError> {
        if (auto list = object.as<List>()) {
            return list->contains(value);
        } else if (auto dictionary = object.as<Dictionary>()) {
            return dictionary->contains(value);
        } else if (auto string = object.as<String>()) {
            if (auto lookup = value.as<String>()) {
                return string->string().find(lookup->string()) != std::string::npos;
            }
            return Error(RuntimeError(location, "expected a string argument"));
        } else if (auto range = object.as<Range>()) {
            if (auto queryRange = value.as<Range>()) {
                return range->contains(*queryRange);
            }
            if (!value.isInteger()) {
                return Error(RuntimeError(location, "expected an integer or range"));
            }
            return range->contains(value.asInteger());
        }
        return Error(RuntimeError(location, "expected a string, list, dictionary, or range"));
    };
    natives[S("{} contains {}")] = MakeStrong<Native>(
        [contains](Location location, Value *values) -> Result<Value, RuntimeError> {
            return contains(location, values[0], values[1]);
        });
    natives[S("{} is in {}")] = MakeStrong<Native>(
        [contains](Location location, Value *values) -> Result<Value, RuntimeError> {
            return contains(location, values[1], values[0]);
        });

    natives[S("{} starts with {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto string = values[0].as<String>()) {
                auto searchString = values[1].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return string->startsWith(*searchString);
            } else if (auto list = values[0].as<List>()) {
                return list->startsWith(values[1]);
            }
            return Error(RuntimeError(location, "expected a string or list"));
        });
    natives[S("{} ends with {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto string = values[0].as<String>()) {
                auto searchString = values[1].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return string->endsWith(*searchString);
            } else if (auto list = values[0].as<List>()) {
                return list->endsWith(values[1]);
            }
            return Error(RuntimeError(location, "expected a string or list"));
        });
    natives[S("item {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto list = values[1].as<List>()) {
                return list->subscript(location, values[0]);
            } else if (auto dictionary = values[1].as<Dictionary>()) {
                return dictionary->subscript(location, values[0]);
            }
            return Error(RuntimeError(location, "expected a list or dictionary"));
        });
    natives[S("insert {} at (the) end of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto list = values[1].as<List>()) {
                list->values().push_back(values[0]);
            } else if (auto string = values[1].as<String>()) {
                string->string() += Concat(values[0]);
            } else {
                return Error(RuntimeError(
                    location, Concat("expected a list or string, got ", values[1].typeName())));
            }
            return values[1];
        });
    natives[S("remove item {} from {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto dictionary = values[1].as<Dictionary>()) {
                dictionary->values().erase(values[0]);
                return dictionary;
            } else if (auto list = values[1].as<List>()) {
                if (!values[0].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer index"));
                }
                auto index = values[0].asInteger();
                list->values().erase(list->values().begin() + index);
                return list;
            }
            return Error(RuntimeError(location, "expected a dictionary"));
        });
    natives[S("(the) (first) offset of {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto text = values[1].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto result = text->findFirst(*searchString);
                return result == std::string::npos ? Value() : Integer(result);
            } else if (auto list = values[1].as<List>()) {
                if (auto result = list->findFirst(values[0])) {
                    return result.value();
                }
                return Value();
            }
            return Error(RuntimeError(location, "expected a string"));
        });
    natives[S("(the) last offset of {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto text = values[1].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string or list"));
                }
                auto result = text->findLast(*searchString);
                return result == std::string::npos ? Value() : Integer(result);
            } else if (auto list = values[1].as<List>()) {
                if (auto result = list->findLast(values[0])) {
                    return result.value();
                }
                return Value();
            }
            return Error(RuntimeError(location, "expected a string or list"));
        });
    natives[S("replace all {} with {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto text = values[2].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto replacementString = values[1].as<String>();
                if (!replacementString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                text->replaceAll(*searchString, *replacementString);
                return text;
            } else if (auto list = values[2].as<List>()) {
                list->replaceAll(values[0], values[1]);
                return list;
            }
            return Error(RuntimeError(location, "expected a string or list"));
        });
    natives[S("replace first {} with {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto text = values[2].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto replacementString = values[1].as<String>();
                if (!replacementString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                text->replaceFirst(*searchString, *replacementString);
                return text;
            } else if (auto list = values[2].as<List>()) {
                list->replaceFirst(values[0], values[1]);
                return list;
            }
            return Error(RuntimeError(location, "expected a string or list"));
        });
    natives[S("replace last {} with {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto text = values[2].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto replacementString = values[1].as<String>();
                if (!replacementString) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                text->replaceLast(*searchString, *replacementString);
                return text;
            } else if (auto list = values[2].as<List>()) {
                list->replaceLast(values[0], values[1]);
                return list;
            }
            return Error(RuntimeError(location, "expected a string or list"));
        });
    return natives;
}();

ModuleMap _dictionaryNatives = []() -> ModuleMap {
    ModuleMap natives;
    natives[S("(the) keys (of) {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto dictionary = values[0].as<Dictionary>();
            if (!dictionary) {
                return Error(RuntimeError(location, "expected a dictionary"));
            }
            auto keys = MakeStrong<List>();
            for (const auto &pair : dictionary->values()) {
                keys->values().push_back(pair.first);
            }
            return keys;
        });
    natives[S("(the) values (of) {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto dictionary = values[0].as<Dictionary>();
            if (!dictionary) {
                return Error(RuntimeError(location, "expected a dictionary"));
            }
            auto valuesList = MakeStrong<List>();
            for (const auto &pair : dictionary->values()) {
                valuesList->values().push_back(pair.second);
            }
            return valuesList;
        });
    natives[S("insert item {} with key {} into {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto dictionary = values[2].as<Dictionary>();
            if (!dictionary) {
                return Error(RuntimeError(location, "expected a dictionary"));
            }
            dictionary->values()[values[1]] = values[0];
            return dictionary;
        });
    return natives;
}();

ModuleMap _listNatives = []() -> ModuleMap {
    static std::random_device rd;
    static std::mt19937 engine(rd());
    auto random = [&](int max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(engine);
    };

    ModuleMap natives;
    natives[S("items {} to {} of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[2].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            if (!values[0].isInteger()) {
                return Error(RuntimeError(location, "expected an integer index"));
            }
            if (!values[1].isInteger()) {
                return Error(RuntimeError(location, "expected an integer index"));
            }
            auto index1 = values[0].asInteger();
            auto index2 = values[1].asInteger();
            return MakeStrong<List>(list->values().begin() + index1,
                                    list->values().begin() + index2 + 1);
        });
    natives[S("(the) mid/middle item of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            return list->values().at(list->values().size() / 2);
        });
    natives[S("(the) last item of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            return list->values().back();
        });
    natives[S("(the) number of items in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            return Integer(list->values().size());
        });
    natives[S("any item of {}")] = MakeStrong<Native>(
        [random](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            return list->values().at(random(list->values().size()));
        });
    natives[S("remove items {} to {} from {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[2].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            if (!values[0].isInteger()) {
                return Error(RuntimeError(location, "expected an integer index"));
            }
            if (!values[1].isInteger()) {
                return Error(RuntimeError(location, "expected an integer index"));
            }
            auto index1 = values[0].asInteger();
            auto index2 = values[1].asInteger();
            list->values().erase(list->values().begin() + index1,
                                 list->values().begin() + index2 + 1);
            return list;
        });
    natives[S("insert {} at index {} into {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[2].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            if (!values[1].isInteger()) {
                return Error(RuntimeError(location, "expected an integer index"));
            }
            auto index = values[1].asInteger();
            list->values().insert(list->values().begin() + index, values[0]);
            return list;
        });
    natives[S("reverse {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            std::reverse(list->values().begin(), list->values().end());
            return list;
        });
    natives[S("reversed {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            return MakeStrong<List>(list->values().rbegin(), list->values().rend());
        });
    natives[S("shuffle {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            std::shuffle(list->values().begin(), list->values().end(), engine);
            return list;
        });
    natives[S("shuffled {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            auto result = MakeStrong<List>(list->values());
            std::shuffle(result->values().begin(), result->values().end(), engine);
            return result;
        });
    return natives;
}();

ModuleMap _stringNatives = []() -> ModuleMap {
    static std::random_device rd;
    static std::mt19937 engine(rd());
    auto random = [&](int max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(engine);
    };

    ModuleMap natives;

    natives[S("insert {} at char/character {} in {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto insertText = values[0].as<String>();
            if (!insertText) {
                return Error(RuntimeError(location, "expected a string"));
            }
            if (!values[1].isInteger()) {
                return Error(RuntimeError(location, "expected an integer"));
            }
            auto text = values[2].as<String>();
            if (!text) {
                return Error(RuntimeError(location, "expected a string"));
            }
            auto chunk = index_chunk(chunk::type::character, values[1].asInteger(), text->string());
            text->string().insert(chunk.begin(), insertText->string().begin(),
                                  insertText->string().end());
            return text;
        });
    natives[S("insert {} at (the) end of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto insertText = values[0].as<String>();
            if (!insertText) {
                return Error(RuntimeError(location, "expected a string"));
            }
            auto text = values[2].as<String>();
            if (!text) {
                return Error(RuntimeError(location, "expected a string"));
            }
            text->string().append(insertText->string());
            return text;
        });
    natives[S("remove all {} from {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto removeText = values[0].as<String>();
            if (!removeText) {
                return Error(RuntimeError(location, "expected a string"));
            }
            auto text = values[1].as<String>();
            if (!text) {
                return Error(RuntimeError(location, "expected a string"));
            }
            text->replaceAll(*removeText, String(""));
            return text;
        });
    natives[S("remove first {} from {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto removeText = values[0].as<String>();
            if (!removeText) {
                return Error(RuntimeError(location, "expected a string"));
            }
            auto text = values[1].as<String>();
            if (!text) {
                return Error(RuntimeError(location, "expected a string"));
            }
            text->replaceFirst(*removeText, String(""));
            return text;
        });
    natives[S("remove last {} from {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto removeText = values[0].as<String>();
            if (!removeText) {
                return Error(RuntimeError(location, "expected a string"));
            }
            auto text = values[1].as<String>();
            if (!text) {
                return Error(RuntimeError(location, "expected a string"));
            }
            text->replaceLast(*removeText, String(""));
            return text;
        });
    auto replaceChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                auto index = values[0].asInteger();
                auto replacement = values[1].as<String>();
                if (!replacement) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto text = values[2].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }

                auto chunk = index_chunk(chunkType, index, text->string());
                text->string().replace(chunk.begin(), chunk.end(), replacement->string());
                return text;
            });
    };
    natives[S("replace char/character {} with {} in {}")] = replaceChunk(chunk::character);
    natives[S("replace word {} with {} in {}")] = replaceChunk(chunk::word);
    natives[S("replace line {} with {} in {}")] = replaceChunk(chunk::line);

    auto replaceChunks = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isInteger() || !values[1].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                auto start = values[0].asInteger();
                auto end = values[1].asInteger();
                auto replacement = values[2].as<String>();
                if (!replacement) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto text = values[3].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto chunk = range_chunk(chunkType, start, end, text->string());
                text->string().replace(chunk.begin(), chunk.end(), replacement->string());
                return text;
            });
    };
    natives[S("replace chars/characters {} to {} with {} in {}")] = replaceChunks(chunk::character);
    natives[S("replace words {} to {} with {} in {}")] = replaceChunks(chunk::word);
    natives[S("replace lines {} to {} with {} in {}")] = replaceChunks(chunk::line);

    auto removeChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                auto index = values[0].asInteger();
                auto text = values[1].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto chunk = index_chunk(chunkType, index, text->string());
                text->string().erase(chunk.begin(), chunk.end());
                return text;
            });
    };
    natives[S("remove char/character {} from {}")] = removeChunk(chunk::character);
    natives[S("remove word {} from {}")] = removeChunk(chunk::word);
    natives[S("remove line {} from {}")] = removeChunk(chunk::line);

    auto removeChunks = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                if (!values[1].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                auto start = values[0].asInteger();
                auto end = values[1].asInteger();
                auto text = values[2].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                auto chunk = range_chunk(chunkType, start, end, text->string());
                text->string().erase(chunk.begin(), chunk.end());
                return text;
            });
    };
    natives[S("remove chars/characters {} to {} from {}")] = removeChunks(chunk::character);
    natives[S("remove words {} to {} from {}")] = removeChunks(chunk::word);
    natives[S("remove lines {} to {} from {}")] = removeChunks(chunk::line);

    auto listOf = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                auto text = values[0].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                std::vector<Value> result;
                size_t index = 0;
                auto chunk = index_chunk(chunkType, index++, text->string());
                while (chunk.begin() < text->string().end()) {
                    result.push_back(Value(chunk.get()));
                    chunk = index_chunk(chunkType, index++, text->string());
                }
                return MakeStrong<List>(result);
            });
    };
    natives[S("(the) list of chars/characters in {}")] = listOf(chunk::character);
    natives[S("(the) list of words in {}")] = listOf(chunk::word);
    natives[S("(the) list of lines in {}")] = listOf(chunk::line);

    auto chunkAt = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                auto index = values[0].asInteger();
                auto text = values[1].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return index_chunk(chunkType, index, text->string()).get();
            });
    };
    natives[S("char/character {} of {}")] = chunkAt(chunk::character);
    natives[S("word {} of {}")] = chunkAt(chunk::word);
    natives[S("line {} of {}")] = chunkAt(chunk::line);

    auto chunksAt = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isInteger() || !values[1].isInteger()) {
                    return Error(RuntimeError(location, "expected an integer"));
                }
                auto start = values[0].asInteger();
                auto end = values[1].asInteger();
                auto text = values[2].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return range_chunk(chunkType, start, end, text->string()).get();
            });
    };
    natives[S("chars/characters {} to {} of {}")] = chunksAt(chunk::character);
    natives[S("words {} to {} of {}")] = chunksAt(chunk::word);
    natives[S("lines {} to {} of {}")] = chunksAt(chunk::line);

    auto anyChunk = [random](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [random, chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                auto text = values[0].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return random_chunk(chunkType, random, text->string()).get();
            });
    };
    natives[S("any char/character of {}")] = anyChunk(chunk::character);
    natives[S("any word of {}")] = anyChunk(chunk::word);
    natives[S("any line of {}")] = anyChunk(chunk::line);

    auto middleChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                auto text = values[0].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return middle_chunk(chunkType, text->string()).get();
            });
    };
    natives[S("(the) mid/middle char/character of {}")] = middleChunk(chunk::character);
    natives[S("(the) mid/middle word of {}")] = middleChunk(chunk::word);
    natives[S("(the) mid/middle line of {}")] = middleChunk(chunk::line);

    auto lastChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                auto text = values[0].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return last_chunk(chunkType, text->string()).get();
            });
    };
    natives[S("(the) last char/character of {}")] = lastChunk(chunk::character);
    natives[S("(the) last word of {}")] = lastChunk(chunk::word);
    natives[S("(the) last line of {}")] = lastChunk(chunk::line);

    auto numberOfChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [chunkType](Location location, Value *values) -> Result<Value, RuntimeError> {
                auto text = values[0].as<String>();
                if (!text) {
                    return Error(RuntimeError(location, "expected a string"));
                }
                return static_cast<Integer>(count_chunk(chunkType, text->string()).count);
            });
    };
    natives[S("(the) number of chars/characters in {}")] = numberOfChunk(chunk::character);
    natives[S("(the) number of words in {}")] = numberOfChunk(chunk::word);
    natives[S("(the) number of lines in {}")] = numberOfChunk(chunk::line);

    return natives;
}();

ModuleMap _rangeNatives = []() -> ModuleMap {
    static std::random_device rd;
    static std::mt19937 engine(rd());
    auto random = [&](int max) {
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(engine);
    };

    ModuleMap natives;
    natives[S("{} up to {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (!values[0].isInteger() || !values[1].isInteger()) {
                return Error(RuntimeError(location, "expected an integer"));
            }
            return MakeStrong<Range>(values[0].asInteger(), values[1].asInteger(), true);
        });
    natives[S("(the) lower bound of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto range = values[0].as<Range>()) {
                return range->start();
            }
            return Error(RuntimeError(location, "expected a range"));
        });
    natives[S("(the) upper bound of {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto range = values[0].as<Range>()) {
                return range->end();
            }
            return Error(RuntimeError(location, "expected a range"));
        });
    natives[S("{} is closed")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto range = values[0].as<Range>()) {
                return range->closed();
            }
            return Error(RuntimeError(location, "expected a range"));
        });
    natives[S("{} overlaps (with) {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto range1 = values[0].as<Range>();
            auto range2 = values[1].as<Range>();
            if (!range1) {
                return Error(RuntimeError(location, "expected a range"));
            }
            if (!range2) {
                return Error(RuntimeError(location, "expected a range"));
            }
            return range1->overlaps(*range2);
        });
    natives[S("(a) random number (in) {}")] = MakeStrong<Native>(
        [random](Location location, Value *values) -> Result<Value, RuntimeError> {
            if (auto range = values[0].as<Range>()) {
                return range->start() +
                       random(range->end() - range->start() + (range->closed() ? 1 : 0));
            }
            return Error(RuntimeError(location, "expected a range"));
        });
    return natives;
}();

ModuleMap _mathNatives = []() -> ModuleMap {
    ModuleMap natives;
    auto basicFunction = [](double (*func)(double)) -> Strong<Native> {
        return MakeStrong<Native>(
            [func](Location location, Value *values) -> Result<Value, RuntimeError> {
                if (!values[0].isNumber()) {
                    return Error(RuntimeError(location, "expected a number"));
                }
                auto argument = values[0].castFloat();
                return func(argument);
            });
    };
    natives[S("(the) sin (of) {}")] = basicFunction(sin);
    natives[S("(the) cos (of) {}")] = basicFunction(cos);
    natives[S("(the) tan (of) {}")] = basicFunction(tan);
    natives[S("(the) atan (of) {}")] = basicFunction(atan);
    natives[S("(the) abs (of) {}")] = basicFunction(fabs);
    natives[S("(the) exp (of) {}")] = basicFunction(exp);
    natives[S("(the) exp2 (of) {}")] = basicFunction(exp2);
    natives[S("(the) expm1 (of) {}")] = basicFunction(expm1);
    natives[S("(the) log2 (of) {}")] = basicFunction(log2);
    natives[S("(the) log10 (of) {}")] = basicFunction(log10);
    natives[S("(the) log (of) {}")] = basicFunction(log);
    natives[S("(the) sqrt (of) {}")] = basicFunction(sqrt);
    natives[S("(the) ceil (of) {}")] = basicFunction(ceil);
    natives[S("(the) floor (of) {}")] = basicFunction(floor);
    natives[S("round {}")] = basicFunction(round);
    natives[S("trunc/truncate {}")] = basicFunction(trunc);

    natives[S("(the) max/maximum (of) {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            if (list->values().size() == 0) {
                return Error(RuntimeError(location, "list is empty"));
            }
            auto first = list->values().front();
            if (!first.isNumber()) {
                return Error(RuntimeError(location, "expected a number"));
            }
            auto max = first.castFloat();
            for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
                if (!it->isNumber()) {
                    return Error(RuntimeError(location, "expected a number"));
                }
                auto value = it->castFloat();
                if (value > max) {
                    max = value;
                }
            }
            return max;
        });
    natives[S("(the) min/minimum (of) {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            if (list->values().size() == 0) {
                return Error(RuntimeError(location, "list is empty"));
            }
            auto first = list->values().front();
            if (!first.isNumber()) {
                return Error(RuntimeError(location, "expected a number"));
            }
            auto min = first.castFloat();
            for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
                if (!it->isNumber()) {
                    return Error(RuntimeError(location, "expected a number"));
                }
                auto value = it->castFloat();
                if (value < min) {
                    min = value;
                }
            }
            return min;
        });
    natives[S("(the) average/avg (of) {}")] =
        MakeStrong<Native>([](Location location, Value *values) -> Result<Value, RuntimeError> {
            auto list = values[0].as<List>();
            if (!list) {
                return Error(RuntimeError(location, "expected a list"));
            }
            if (list->values().size() == 0) {
                return Error(RuntimeError(location, "list is empty"));
            }
            auto first = list->values().front();
            if (!first.isNumber()) {
                return Error(RuntimeError(location, "expected a number"));
            }
            auto sum = first.castFloat();
            for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
                if (!it->isNumber()) {
                    return Error(RuntimeError(location, "expected a number"));
                }
                sum = sum + it->castFloat();
            }
            return sum / list->values().size();
        });
    return natives;
}();

ModuleMap _natives = []() -> ModuleMap {
    ModuleMap natives;
    natives.insert(_coreNatives.begin(), _coreNatives.end());
    natives.insert(_commonNatives.begin(), _commonNatives.end());
    natives.insert(_dictionaryNatives.begin(), _dictionaryNatives.end());
    natives.insert(_listNatives.begin(), _listNatives.end());
    natives.insert(_stringNatives.begin(), _stringNatives.end());
    natives.insert(_rangeNatives.begin(), _rangeNatives.end());
    natives.insert(_mathNatives.begin(), _mathNatives.end());
    return natives;
}();

std::vector<Signature> Core::signatures() const {
    std::vector<Signature> signatures;
    for (const auto &pair : _natives) {
        signatures.push_back(pair.first);
    }
    return signatures;
}

Mapping<std::string, Strong<Native>> Core::functions() const {
    Mapping<std::string, Strong<Native>> natives;
    for (const auto &native : _natives) {
        natives.insert({native.first.name(), native.second});
    }
    return natives;
}

SIF_NAMESPACE_END