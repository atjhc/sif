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

#include <charconv>
#include <format>
#include <random>

SIF_NAMESPACE_BEGIN

#define N(X) MakeStrong<Native>(X)

using ModuleMap = Mapping<Signature, Strong<Native>, Signature::Hash>;
static Signature S(const char *signature) { return Signature::Make(signature).value(); }

static void _core(ModuleMap &natives, std::ostream &out, std::istream &in, std::ostream &err) {
    natives[S("the language version")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return Value(std::string(Version));
        });
    natives[S("the language major version")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return Value(MajorVersion);
        });
    natives[S("the language minor version")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return Value(MinorVersion);
        });
    natives[S("the language patch version")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return Value(PatchVersion);
        });
    natives[S("the error")] =
        MakeStrong<Native>([](CallFrame &frame, SourceLocation location,
                              Value *values) -> Result<Value, Error> { return frame.error; });
    natives[S("error with {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return Fail(Error(location, values[0]));
        });
    natives[S("quit")] = MakeStrong<Native>([](CallFrame &frame, SourceLocation location,
                                               Value *values) -> Result<Value, Error> { exit(0); });
    natives[S("quit with {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            exit(values[0].asInteger());
        });
    natives[S("write {}")] = MakeStrong<Native>(
        [&out](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                out << Join(list->values(), " ");
            } else {
                out << values[0];
            }
            return Value();
        });
    natives[S("write error {}")] = MakeStrong<Native>(
        [&err](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                err << Join(list->values(), " ");
            } else {
                err << values[0];
            }
            return Value();
        });
    natives[S("print {}")] = MakeStrong<Native>(
        [&out](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                out << Join(list->values(), " ");
            } else {
                out << values[0];
            }
            out << std::endl;
            return Value();
        });
    natives[S("print error {}")] = MakeStrong<Native>(
        [&err](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (const auto &list = values[0].as<List>()) {
                for (const auto &item : list->values()) {
                    err << item;
                }
            } else {
                err << values[0];
            }
            err << std::endl;
            return Value();
        });
    natives[S("get {}")] =
        MakeStrong<Native>([](CallFrame &frame, SourceLocation location,
                              Value *values) -> Result<Value, Error> { return values[0]; });
    natives[S("read (a) word")] = MakeStrong<Native>(
        [&in](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::string input;
            in >> input;
            return input;
        });
    natives[S("read (a) line")] = MakeStrong<Native>(
        [&in](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::string input;
            std::getline(in, input);
            return input;
        });
    natives[S("read (a) character")] = MakeStrong<Native>(
        [&in](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::istreambuf_iterator<char> it(in.rdbuf());
            std::istreambuf_iterator<char> eos;
            std::string result;
            try {
                char32_t input = utf8::next(it, eos);
                result = utf8::utf32to8(std::u32string_view(&input, 1));
            } catch (const utf8::exception &exception) {
                return Fail(Error(location, exception.what()));
            }
            return result;
        });
    natives[S("(the) debug description of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return values[0].debugDescription();
        });
    natives[S("(the) description of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            std::ostringstream ss;
            ss << values[0];
            return ss.str();
        });
    natives[S("(the) hash value of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return Integer(Value::Hash()(values[0]));
        });
    natives[S("(the) type name of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            return lowercase(values[0].typeName());
        });
    natives[S("(a) copy of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto copyable = values[0].as<Copyable>()) {
                return copyable->copy();
            }
            return values[0];
        });
}

static auto _sort_list(CallFrame &frame, SourceLocation location, Strong<List> list)
    -> Result<Value, Error> {
    try {
        std::sort(list->values().begin(), list->values().end(),
                  [&location](const Value &a, const Value &b) {
                      if (a.isInteger() && b.isInteger()) {
                          return a.asInteger() < b.asInteger();
                      } else if (a.isNumber() && b.isNumber()) {
                          return a.castFloat() < b.castFloat();
                      }
                      if (a.type() != b.type() || a.typeName() != b.typeName()) {
                          throw Error(location, "can't compare {} {} and {} {}", a.typeName(),
                                      a.toString(), b.typeName(), b.toString());
                      }
                      return true;
                  });
    } catch (const Error &error) {
        return Fail(error);
    }
    return Value(list);
}

static auto _sort_string(CallFrame &frame, SourceLocation location, Strong<String> string)
    -> Result<Value, Error> {
    return Fail(Error(location, "not supported"));
}

static auto _sort_dictionary(CallFrame &frame, SourceLocation location,
                             Strong<Dictionary> dictionary) -> Result<Value, Error> {
    return Fail(Error(location, "not supported"));
}

static auto _sort_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto list = values[0].as<List>()) {
        return _sort_list(frame, location, list);
    } else if (auto string = values[0].as<String>()) {
        return _sort_string(frame, location, string);
    } else if (auto dictionary = values[0].as<Dictionary>()) {
        return _sort_dictionary(frame, location, dictionary);
    }
    return values[0];
}

static void _common(ModuleMap &natives) {
    natives[S("(the) size of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
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
                return Fail(Error(location, "expected a string, list, dictionary, or range"));
            }
            return static_cast<long>(size);
        });
    auto contains = [](SourceLocation location, Value object, Value value) -> Result<Value, Error> {
        if (auto list = object.as<List>()) {
            return list->contains(value);
        } else if (auto dictionary = object.as<Dictionary>()) {
            return dictionary->contains(value);
        } else if (auto string = object.as<String>()) {
            if (auto lookup = value.as<String>()) {
                return string->string().find(lookup->string()) != std::string::npos;
            }
            return Fail(Error(location, "expected a string argument"));
        } else if (auto range = object.as<Range>()) {
            if (auto queryRange = value.as<Range>()) {
                return range->contains(*queryRange);
            }
            if (!value.isInteger()) {
                return Fail(Error(location, "expected an integer or range"));
            }
            return range->contains(value.asInteger());
        }
        return Fail(Error(location, "expected a string, list, dictionary, or range"));
    };
    natives[S("{} contains {}")] =
        MakeStrong<Native>([contains](CallFrame &frame, SourceLocation location,
                                      Value *values) -> Result<Value, Error> {
            return contains(location, values[0], values[1]);
        });
    natives[S("{} is in {}")] =
        MakeStrong<Native>([contains](CallFrame &frame, SourceLocation location,
                                      Value *values) -> Result<Value, Error> {
            return contains(location, values[1], values[0]);
        });

    natives[S("{} starts with {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto string = values[0].as<String>()) {
                auto searchString = values[1].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string"));
                }
                return string->startsWith(*searchString);
            } else if (auto list = values[0].as<List>()) {
                return list->startsWith(values[1]);
            }
            return Fail(Error(location, "expected a string or list"));
        });
    natives[S("{} ends with {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto string = values[0].as<String>()) {
                auto searchString = values[1].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string"));
                }
                return string->endsWith(*searchString);
            } else if (auto list = values[0].as<List>()) {
                return list->endsWith(values[1]);
            }
            return Fail(Error(location, "expected a string or list"));
        });
    natives[S("item {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto list = values[1].as<List>()) {
                return list->subscript(location, values[0]);
            } else if (auto dictionary = values[1].as<Dictionary>()) {
                return dictionary->subscript(location, values[0]);
            }
            return Fail(Error(location, "expected a list or dictionary"));
        });
    natives[S("insert {} at (the) end of {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto list = values[1].as<List>()) {
                list->values().push_back(values[0]);
            } else if (auto string = values[1].as<String>()) {
                auto insertText = values[0].as<String>();
                if (!insertText) {
                    return Fail(Error(location, "expected a string"));
                }
                string->string().append(insertText->string());
            } else {
                return Fail(Error(location,
                                  Concat("expected a list or string, got ", values[1].typeName())));
            }
            return values[1];
        });
    natives[S("remove item {} from {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto dictionary = values[1].as<Dictionary>()) {
                dictionary->values().erase(values[0]);
                return dictionary;
            } else if (auto list = values[1].as<List>()) {
                if (!values[0].isInteger()) {
                    return Fail(Error(location, "expected an integer index"));
                }
                auto index = values[0].asInteger();
                list->values().erase(list->values().begin() + index);
                return list;
            }
            return Fail(Error(location, "expected a dictionary"));
        });
    natives[S("(the) (first) offset of {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto text = values[1].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string"));
                }
                auto result = text->findFirst(*searchString);
                return result == std::string::npos ? Value() : Integer(result);
            } else if (auto list = values[1].as<List>()) {
                if (auto result = list->findFirst(values[0])) {
                    return result.value();
                }
                return Value();
            }
            return Fail(Error(location, "expected a string"));
        });
    natives[S("(the) last offset of {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto text = values[1].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string or list"));
                }
                auto result = text->findLast(*searchString);
                return result == std::string::npos ? Value() : Integer(result);
            } else if (auto list = values[1].as<List>()) {
                if (auto result = list->findLast(values[0])) {
                    return result.value();
                }
                return Value();
            }
            return Fail(Error(location, "expected a string or list"));
        });
    natives[S("replace all {} with {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto text = values[2].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string"));
                }
                auto replacementString = values[1].as<String>();
                if (!replacementString) {
                    return Fail(Error(location, "expected a string"));
                }
                text->replaceAll(*searchString, *replacementString);
                return text;
            } else if (auto list = values[2].as<List>()) {
                list->replaceAll(values[0], values[1]);
                return list;
            }
            return Fail(Error(location, "expected a string or list"));
        });
    natives[S("replace first {} with {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto text = values[2].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string"));
                }
                auto replacementString = values[1].as<String>();
                if (!replacementString) {
                    return Fail(Error(location, "expected a string"));
                }
                text->replaceFirst(*searchString, *replacementString);
                return text;
            } else if (auto list = values[2].as<List>()) {
                list->replaceFirst(values[0], values[1]);
                return list;
            }
            return Fail(Error(location, "expected a string or list"));
        });
    natives[S("replace last {} with {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto text = values[2].as<String>()) {
                auto searchString = values[0].as<String>();
                if (!searchString) {
                    return Fail(Error(location, "expected a string"));
                }
                auto replacementString = values[1].as<String>();
                if (!replacementString) {
                    return Fail(Error(location, "expected a string"));
                }
                text->replaceLast(*searchString, *replacementString);
                return text;
            } else if (auto list = values[2].as<List>()) {
                list->replaceLast(values[0], values[1]);
                return list;
            }
            return Fail(Error(location, "expected a string or list"));
        });
    natives[S("sort {}")] = N(_sort_T);
}

static auto _T_as_an_integer(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (values[0].isNumber()) {
        return Value(values[0].castInteger());
    }
    if (auto castable = values[0].as<NumberCastable>()) {
        return castable->castInteger();
    }
    return Fail(Error(location, "can't convert this value to a number"));
}

static auto _T_as_a_number(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (values[0].isNumber()) {
        return Value(values[0].castFloat());
    }
    if (auto castable = values[0].as<NumberCastable>()) {
        return castable->castFloat();
    }
    return Fail(Error(location, "can't convert this value to a number"));
}

static auto _T_as_a_string(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return values[0].toString();
}

static void _types(ModuleMap &natives) {
    natives[S("{} as a/an integer")] = N(_T_as_an_integer);
    natives[S("{} as a/an number")] = N(_T_as_a_number);
    natives[S("{} as a/an string")] = N(_T_as_a_string);
}

static void _dictionary(ModuleMap &natives) {
    natives[S("(the) keys (of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto dictionary = values[0].as<Dictionary>();
            if (!dictionary) {
                return Fail(Error(location, "expected a dictionary"));
            }
            auto keys = MakeStrong<List>();
            for (const auto &pair : dictionary->values()) {
                keys->values().push_back(pair.first);
            }
            return keys;
        });
    natives[S("(the) values (of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto dictionary = values[0].as<Dictionary>();
            if (!dictionary) {
                return Fail(Error(location, "expected a dictionary"));
            }
            auto valuesList = MakeStrong<List>();
            for (const auto &pair : dictionary->values()) {
                valuesList->values().push_back(pair.second);
            }
            return valuesList;
        });
    natives[S("insert item {} with key {} into {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto dictionary = values[2].as<Dictionary>();
            if (!dictionary) {
                return Fail(Error(location, "expected a dictionary"));
            }
            dictionary->values()[values[1]] = values[0];
            return dictionary;
        });
}

static void _list(ModuleMap &natives, std::mt19937_64 &engine,
                  std::function<Integer(Integer)> randomInteger) {
    natives[S("items {} to {} (in/of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[2].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer index"));
            }
            if (!values[1].isInteger()) {
                return Fail(Error(location, "expected an integer index"));
            }
            auto index1 = values[0].asInteger();
            auto index2 = values[1].asInteger();
            return MakeStrong<List>(list->values().begin() + index1,
                                    list->values().begin() + index2 + 1);
        });
    natives[S("(the) mid/middle item (in/of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            return list->values().at(list->values().size() / 2);
        });
    natives[S("(the) last item (in/of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            return list->values().back();
        });
    natives[S("(the) number of items (in/of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            return Integer(list->values().size());
        });
    natives[S("any item (in/of) {}")] =
        MakeStrong<Native>([randomInteger](CallFrame &frame, SourceLocation location,
                                           Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            return list->values().at(randomInteger(list->values().size()));
        });
    natives[S("remove items {} to {} from {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[2].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer index"));
            }
            if (!values[1].isInteger()) {
                return Fail(Error(location, "expected an integer index"));
            }
            auto index1 = values[0].asInteger();
            auto index2 = values[1].asInteger();
            list->values().erase(list->values().begin() + index1,
                                 list->values().begin() + index2 + 1);
            return list;
        });
    natives[S("insert {} at index {} into {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[2].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            if (!values[1].isInteger()) {
                return Fail(Error(location, "expected an integer index"));
            }
            auto index = values[1].asInteger();
            list->values().insert(list->values().begin() + index, values[0]);
            return list;
        });
    natives[S("reverse {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            std::reverse(list->values().begin(), list->values().end());
            return list;
        });
    natives[S("reversed {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            return MakeStrong<List>(list->values().rbegin(), list->values().rend());
        });
    natives[S("shuffle {}")] =
        MakeStrong<Native>([&engine](CallFrame &frame, SourceLocation location,
                                     Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            std::shuffle(list->values().begin(), list->values().end(), engine);
            return list;
        });
    natives[S("shuffled {}")] =
        MakeStrong<Native>([&engine](CallFrame &frame, SourceLocation location,
                                     Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            auto result = MakeStrong<List>(list->values());
            std::shuffle(result->values().begin(), result->values().end(), engine);
            return result;
        });
    natives[S("join {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            std::ostringstream str;
            for (auto it = list->values().begin(); it < list->values().end(); it++) {
                str << it->toString();
            }
            return str.str();
        });
    natives[S("join {} using {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            auto joinString = values[1].as<String>();
            if (!joinString) {
                return Fail(Error(location, "expected a string"));
            }
            std::ostringstream str;
            for (auto it = list->values().begin(); it < list->values().end(); it++) {
                str << it->toString();
                if (it + 1 < list->values().end()) {
                    str << joinString->string();
                }
            }
            return str.str();
        });
}

static void _string(ModuleMap &natives, std::mt19937_64 &engine,
                    std::function<Integer(Integer)> randomInteger) {
    natives[S("insert {} at char/character {} in {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto insertText = values[0].as<String>();
            if (!insertText) {
                return Fail(Error(location, "expected a string"));
            }
            if (!values[1].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto text = values[2].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            auto chunk = index_chunk(chunk::type::character, values[1].asInteger(), text->string());
            text->string().insert(chunk.begin(), insertText->string().begin(),
                                  insertText->string().end());
            return text;
        });
    natives[S("remove all {} from {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto removeText = values[0].as<String>();
            if (!removeText) {
                return Fail(Error(location, "expected a string"));
            }
            auto text = values[1].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            text->replaceAll(*removeText, String(""));
            return text;
        });
    natives[S("remove first {} from {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto removeText = values[0].as<String>();
            if (!removeText) {
                return Fail(Error(location, "expected a string"));
            }
            auto text = values[1].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            text->replaceFirst(*removeText, String(""));
            return text;
        });
    natives[S("remove last {} from {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto removeText = values[0].as<String>();
            if (!removeText) {
                return Fail(Error(location, "expected a string"));
            }
            auto text = values[1].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            text->replaceLast(*removeText, String(""));
            return text;
        });
    auto replaceChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto index = values[0].asInteger();
            auto replacement = values[1].as<String>();
            if (!replacement) {
                return Fail(Error(location, "expected a string"));
            }
            auto text = values[2].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
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
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger() || !values[1].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto start = values[0].asInteger();
            auto end = values[1].asInteger();
            auto replacement = values[2].as<String>();
            if (!replacement) {
                return Fail(Error(location, "expected a string"));
            }
            auto text = values[3].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
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
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto index = values[0].asInteger();
            auto text = values[1].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
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
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            if (!values[1].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto start = values[0].asInteger();
            auto end = values[1].asInteger();
            auto text = values[2].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
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
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            auto text = values[0].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
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
    natives[S("(the) list of chars/characters (in/of) {}")] = listOf(chunk::character);
    natives[S("(the) list of words (in/of) {}")] = listOf(chunk::word);
    natives[S("(the) list of lines (in/of) {}")] = listOf(chunk::line);

    auto chunkAt = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto index = values[0].asInteger();
            auto text = values[1].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            return index_chunk(chunkType, index, text->string()).get();
        });
    };
    natives[S("char/character {} in/of {}")] = chunkAt(chunk::character);
    natives[S("word {} in/of {}")] = chunkAt(chunk::word);
    natives[S("line {} in/of {}")] = chunkAt(chunk::line);

    auto chunksAt = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger() || !values[1].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            auto start = values[0].asInteger();
            auto end = values[1].asInteger();
            auto text = values[2].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            return range_chunk(chunkType, start, end, text->string()).get();
        });
    };
    natives[S("chars/characters {} to {} in/of {}")] = chunksAt(chunk::character);
    natives[S("words {} to {} in/of {}")] = chunksAt(chunk::word);
    natives[S("lines {} to {} in/of {}")] = chunksAt(chunk::line);

    auto anyChunk = [randomInteger](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>(
            [randomInteger, chunkType](CallFrame &frame, SourceLocation location,
                                       Value *values) -> Result<Value, Error> {
                auto text = values[0].as<String>();
                if (!text) {
                    return Fail(Error(location, "expected a string"));
                }
                return random_chunk(chunkType, randomInteger, text->string()).get();
            });
    };
    natives[S("any char/character in/of {}")] = anyChunk(chunk::character);
    natives[S("any word in/of {}")] = anyChunk(chunk::word);
    natives[S("any line in/of {}")] = anyChunk(chunk::line);

    auto middleChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            auto text = values[0].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            return middle_chunk(chunkType, text->string()).get();
        });
    };
    natives[S("(the) mid/middle char/character in/of {}")] = middleChunk(chunk::character);
    natives[S("(the) mid/middle word in/of {}")] = middleChunk(chunk::word);
    natives[S("(the) mid/middle line in/of {}")] = middleChunk(chunk::line);

    auto lastChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            auto text = values[0].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            return last_chunk(chunkType, text->string()).get();
        });
    };
    natives[S("(the) last char/character in/of {}")] = lastChunk(chunk::character);
    natives[S("(the) last word in/of {}")] = lastChunk(chunk::word);
    natives[S("(the) last line in/of {}")] = lastChunk(chunk::line);

    auto numberOfChunk = [](chunk::type chunkType) -> Strong<Native> {
        return MakeStrong<Native>([chunkType](CallFrame &frame, SourceLocation location,
                                              Value *values) -> Result<Value, Error> {
            auto text = values[0].as<String>();
            if (!text) {
                return Fail(Error(location, "expected a string"));
            }
            return static_cast<Integer>(count_chunk(chunkType, text->string()).count);
        });
    };
    natives[S("(the) number of chars/characters (in/of) {}")] = numberOfChunk(chunk::character);
    natives[S("(the) number of words (in/of) {}")] = numberOfChunk(chunk::word);
    natives[S("(the) number of lines (in/of) {}")] = numberOfChunk(chunk::line);
}

static void _range(ModuleMap &natives, std::mt19937_64 &engine,
                   std::function<Integer(Integer)> randomInteger) {
    natives[S("{} up to {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (!values[0].isInteger() || !values[1].isInteger()) {
                return Fail(Error(location, "expected an integer"));
            }
            return MakeStrong<Range>(values[0].asInteger(), values[1].asInteger(), true);
        });
    natives[S("(the) lower bound (in/of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto range = values[0].as<Range>()) {
                return range->start();
            }
            return Fail(Error(location, "expected a range"));
        });
    natives[S("(the) upper bound (in/of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto range = values[0].as<Range>()) {
                return range->end();
            }
            return Fail(Error(location, "expected a range"));
        });
    natives[S("{} is closed")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (auto range = values[0].as<Range>()) {
                return range->closed();
            }
            return Fail(Error(location, "expected a range"));
        });
    natives[S("{} overlaps (with) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto range1 = values[0].as<Range>();
            auto range2 = values[1].as<Range>();
            if (!range1) {
                return Fail(Error(location, "expected a range"));
            }
            if (!range2) {
                return Fail(Error(location, "expected a range"));
            }
            return range1->overlaps(*range2);
        });
    natives[S("(a) random number (in/of) {}")] =
        MakeStrong<Native>([randomInteger](CallFrame &frame, SourceLocation location,
                                           Value *values) -> Result<Value, Error> {
            if (auto range = values[0].as<Range>()) {
                return range->start() +
                       randomInteger(range->end() - range->start() + (range->closed() ? 1 : 0));
            }
            return Fail(Error(location, "expected a range"));
        });
}

static void _math(ModuleMap &natives) {
    auto basicFunction = [](double (*func)(double)) -> Strong<Native> {
        return MakeStrong<Native>([func](CallFrame &frame, SourceLocation location,
                                         Value *values) -> Result<Value, Error> {
            if (!values[0].isNumber()) {
                return Fail(Error(location, "expected a number"));
            }
            auto argument = values[0].castFloat();
            return func(argument);
        });
    };
    natives[S("(the) abs (of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (!values[0].isNumber()) {
                return Fail(Error(location, "expected a number"));
            }
            if (values[0].isFloat()) {
                return std::fabs(values[0].asFloat());
            }
            return std::abs(values[0].asInteger());
        });
    natives[S("(the) sin (of) {}")] = basicFunction(sin);
    natives[S("(the) cos (of) {}")] = basicFunction(cos);
    natives[S("(the) tan (of) {}")] = basicFunction(tan);
    natives[S("(the) atan (of) {}")] = basicFunction(atan);
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

    natives[S("(the) max/maximum (value) (of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            if (list->values().size() == 0) {
                return Fail(Error(location, "list is empty"));
            }
            auto first = list->values().front();
            if (!first.isNumber()) {
                return Fail(Error(location, "expected a number"));
            }
            auto max = first.castFloat();
            for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
                if (!it->isNumber()) {
                    return Fail(Error(location, "expected a number"));
                }
                auto value = it->castFloat();
                if (value > max) {
                    max = value;
                }
            }
            return max;
        });
    natives[S("(the) min/minimum (value) (of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            if (list->values().size() == 0) {
                return Fail(Error(location, "list is empty"));
            }
            auto first = list->values().front();
            if (!first.isNumber()) {
                return Fail(Error(location, "expected a number"));
            }
            auto min = first.castFloat();
            for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
                if (!it->isNumber()) {
                    return Fail(Error(location, "expected a number"));
                }
                auto value = it->castFloat();
                if (value < min) {
                    min = value;
                }
            }
            return min;
        });
    natives[S("(the) average/avg (of) {}")] = MakeStrong<Native>(
        [](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            auto list = values[0].as<List>();
            if (!list) {
                return Fail(Error(location, "expected a list"));
            }
            if (list->values().size() == 0) {
                return Fail(Error(location, "list is empty"));
            }
            auto first = list->values().front();
            if (!first.isNumber()) {
                return Fail(Error(location, "expected a number"));
            }
            auto sum = first.castFloat();
            for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
                if (!it->isNumber()) {
                    return Fail(Error(location, "expected a number"));
                }
                sum = sum + it->castFloat();
            }
            return sum / list->values().size();
        });
}

Core::Core(const CoreConfig &config) : _config(config) {
    _core(_natives, _config.out, _config.in, _config.err);
    _common(_natives);
    _types(_natives);
    _dictionary(_natives);
    _list(_natives, _config.engine, _config.randomInteger);
    _string(_natives, _config.engine, _config.randomInteger);
    _range(_natives, _config.engine, _config.randomInteger);
    _math(_natives);
}

std::vector<Signature> Core::signatures() const {
    std::vector<Signature> signatures;
    for (const auto &pair : _natives) {
        signatures.push_back(pair.first);
    }
    return signatures;
}

Mapping<std::string, Value> Core::values() const {
    Mapping<std::string, Value> values;
    for (const auto &native : _natives) {
        values.insert({native.first.name(), native.second});
    }
    return values;
}

SIF_NAMESPACE_END
