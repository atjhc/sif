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

#include "extern/utf8.h"
#include "utilities/chunk.h"

#include <charconv>
#include <format>
#include <random>

SIF_NAMESPACE_BEGIN

#define N(X) MakeStrong<Native>(X)

using ModuleMap = Mapping<Signature, Strong<Native>, Signature::Hash>;
static Signature S(const char *signature) { return Signature::Make(signature).value(); }

static auto _the_language_version(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return Value(std::string(Version));
}

static auto _the_language_major_version(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return Value(MajorVersion);
}

static auto _the_language_minor_version(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return Value(MinorVersion);
}

static auto _the_language_patch_version(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return Value(PatchVersion);
}

static auto _the_error(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return frame.error;
}

static auto _error_with_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return Fail(Error(location, values[0]));
}

static auto _quit(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    exit(0);
}

static auto _quit_with_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (!values[0].isInteger()) {
        return Fail(Error(location, "expected an integer"));
    }
    exit(static_cast<int>(values[0].asInteger()));
}

static auto _get_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return values[0];
}

static auto _the_description_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    std::ostringstream ss;
    ss << values[0];
    return ss.str();
}

static auto _the_debug_description_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return values[0].debugDescription();
}

static auto _the_hash_value_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return Integer(Value::Hash()(values[0]));
}

static auto _the_type_name_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return lowercase(values[0].typeName());
}

static auto _a_copy_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto copyable = values[0].as<Copyable>()) {
        return copyable->copy();
    }
    return values[0];
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

static auto _the_size_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _contains(SourceLocation location, Value object, Value value) -> Result<Value, Error> {
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
}

static auto _T_contains_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return _contains(location, values[0], values[1]);
}

static auto _T_is_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    return _contains(location, values[1], values[0]);
}

static auto _T_starts_with_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _T_ends_with_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _item_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto list = values[1].as<List>()) {
        return list->subscript(location, values[0]);
    } else if (auto dictionary = values[1].as<Dictionary>()) {
        return dictionary->subscript(location, values[0]);
    }
    return Fail(Error(location, "expected a list or dictionary"));
}

static auto _insert_T_at_the_end_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto list = values[1].as<List>()) {
        list->values().push_back(values[0]);
    } else if (auto string = values[1].as<String>()) {
        auto insertText = values[0].as<String>();
        if (!insertText) {
            return Fail(Error(location, "expected a string"));
        }
        string->string().append(insertText->string());
    } else {
        return Fail(
            Error(location, Concat("expected a list or string, got ", values[1].typeName())));
    }
    return values[1];
}

static auto _remove_item_T_from_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _the_first_offset_of_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _the_last_offset_of_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _replace_all_T_with_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _replace_first_T_with_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _replace_last_T_with_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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

static auto _the_keys_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto dictionary = values[0].as<Dictionary>();
    if (!dictionary) {
        return Fail(Error(location, "expected a dictionary"));
    }
    auto keys = MakeStrong<List>();
    for (const auto &pair : dictionary->values()) {
        keys->values().push_back(pair.first);
    }
    return keys;
}

static auto _the_values_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto dictionary = values[0].as<Dictionary>();
    if (!dictionary) {
        return Fail(Error(location, "expected a dictionary"));
    }
    auto valuesList = MakeStrong<List>();
    for (const auto &pair : dictionary->values()) {
        valuesList->values().push_back(pair.second);
    }
    return valuesList;
}

static auto _insert_item_T_with_key_T_into_T(CallFrame &frame, SourceLocation location,
                                             Value *values) -> Result<Value, Error> {
    auto dictionary = values[2].as<Dictionary>();
    if (!dictionary) {
        return Fail(Error(location, "expected a dictionary"));
    }
    dictionary->values()[values[1]] = values[0];
    return dictionary;
}

static auto _items_T_to_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
    return MakeStrong<List>(list->values().begin() + index1, list->values().begin() + index2 + 1);
}

static auto _the_middle_item_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto list = values[0].as<List>();
    if (!list) {
        return Fail(Error(location, "expected a list"));
    }
    return list->values().at(list->values().size() / 2);
}

static auto _the_last_item_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto list = values[0].as<List>();
    if (!list) {
        return Fail(Error(location, "expected a list"));
    }
    return list->values().back();
}

static auto _the_number_of_items_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto list = values[0].as<List>();
    if (!list) {
        return Fail(Error(location, "expected a list"));
    }
    return Integer(list->values().size());
}

static auto _any_item_in_T(std::function<Integer(Integer)> randomInteger)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [randomInteger](CallFrame &frame, SourceLocation location,
                           Value *values) -> Result<Value, Error> {
        auto list = values[0].as<List>();
        if (!list) {
            return Fail(Error(location, "expected a list"));
        }
        return list->values().at(randomInteger(list->values().size()));
    };
}

static auto _remove_items_T_to_T_from_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
    list->values().erase(list->values().begin() + index1, list->values().begin() + index2 + 1);
    return list;
}

static auto _insert_T_at_index_T_into_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _reverse_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto list = values[0].as<List>();
    if (!list) {
        return Fail(Error(location, "expected a list"));
    }
    std::reverse(list->values().begin(), list->values().end());
    return list;
}

static auto _reversed_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto list = values[0].as<List>();
    if (!list) {
        return Fail(Error(location, "expected a list"));
    }
    return MakeStrong<List>(list->values().rbegin(), list->values().rend());
}

static auto _shuffle_T(std::mt19937_64 &engine)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [&engine](CallFrame &frame, SourceLocation location,
                     Value *values) -> Result<Value, Error> {
        auto list = values[0].as<List>();
        if (!list) {
            return Fail(Error(location, "expected a list"));
        }
        std::shuffle(list->values().begin(), list->values().end(), engine);
        return list;
    };
}

static auto _shuffled_T(std::mt19937_64 &engine)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [&engine](CallFrame &frame, SourceLocation location,
                     Value *values) -> Result<Value, Error> {
        auto list = values[0].as<List>();
        if (!list) {
            return Fail(Error(location, "expected a list"));
        }
        auto result = MakeStrong<List>(list->values());
        std::shuffle(result->values().begin(), result->values().end(), engine);
        return result;
    };
}

static auto _join_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto list = values[0].as<List>();
    if (!list) {
        return Fail(Error(location, "expected a list"));
    }
    std::ostringstream str;
    for (auto it = list->values().begin(); it < list->values().end(); it++) {
        str << it->toString();
    }
    return str.str();
}

static auto _join_T_using_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _insert_T_at_character_T_in_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
    text->string().insert(chunk.begin(), insertText->string().begin(), insertText->string().end());
    return text;
}

static auto _remove_all_T_from_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _remove_first_T_from_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _remove_last_T_from_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _replace_chunk_T_with_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _replace_chunks_T_to_T_with_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _remove_chunk_T_from_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _remove_chunks_T_to_T_from_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _the_list_of_chunks_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _chunk_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _chunks_T_to_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
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
    };
}

static auto _any_chunk_in_T(chunk::type chunkType, std::function<Integer(Integer)> randomInteger)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType, randomInteger](CallFrame &frame, SourceLocation location,
                                      Value *values) -> Result<Value, Error> {
        auto text = values[0].as<String>();
        if (!text) {
            return Fail(Error(location, "expected a string"));
        }
        return random_chunk(chunkType, randomInteger, text->string()).get();
    };
}

static auto _the_middle_chunk_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
                       Value *values) -> Result<Value, Error> {
        auto text = values[0].as<String>();
        if (!text) {
            return Fail(Error(location, "expected a string"));
        }
        return middle_chunk(chunkType, text->string()).get();
    };
}

static auto _the_last_chunk_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
                       Value *values) -> Result<Value, Error> {
        auto text = values[0].as<String>();
        if (!text) {
            return Fail(Error(location, "expected a string"));
        }
        return last_chunk(chunkType, text->string()).get();
    };
}

static auto _the_number_of_chunks_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [chunkType](CallFrame &frame, SourceLocation location,
                       Value *values) -> Result<Value, Error> {
        auto text = values[0].as<String>();
        if (!text) {
            return Fail(Error(location, "expected a string"));
        }
        return static_cast<Integer>(count_chunk(chunkType, text->string()).count);
    };
}

static auto _T_up_to_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (!values[0].isInteger() || !values[1].isInteger()) {
        return Fail(Error(location, "expected an integer"));
    }
    return MakeStrong<Range>(values[0].asInteger(), values[1].asInteger(), true);
}

static auto _the_lower_bound_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto range = values[0].as<Range>()) {
        return range->start();
    }
    return Fail(Error(location, "expected a range"));
}

static auto _the_upper_bound_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto range = values[0].as<Range>()) {
        return range->end();
    }
    return Fail(Error(location, "expected a range"));
}

static auto _T_is_closed(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (auto range = values[0].as<Range>()) {
        return range->closed();
    }
    return Fail(Error(location, "expected a range"));
}

static auto _T_overlaps_with_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    auto range1 = values[0].as<Range>();
    auto range2 = values[1].as<Range>();
    if (!range1) {
        return Fail(Error(location, "expected a range"));
    }
    if (!range2) {
        return Fail(Error(location, "expected a range"));
    }
    return range1->overlaps(*range2);
}

static auto _a_random_number_in_T(std::function<Integer(Integer)> randomInteger)
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return [randomInteger](CallFrame &frame, SourceLocation location,
                           Value *values) -> Result<Value, Error> {
        if (auto range = values[0].as<Range>()) {
            return range->start() +
                   randomInteger(range->end() - range->start() + (range->closed() ? 1 : 0));
        }
        return Fail(Error(location, "expected a range"));
    };
}

static auto _the_func_of_T(double (*func)(double))
    -> std::function<Result<Value, Error>(CallFrame &, SourceLocation, Value *)> {
    return
        [func](CallFrame &frame, SourceLocation location, Value *values) -> Result<Value, Error> {
            if (!values[0].isNumber()) {
                return Fail(Error(location, "expected a number"));
            }
            auto argument = values[0].castFloat();
            return func(argument);
        };
}

static auto _the_abs_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
    if (!values[0].isNumber()) {
        return Fail(Error(location, "expected a number"));
    }
    if (values[0].isFloat()) {
        return std::fabs(values[0].asFloat());
    }
    return std::abs(values[0].asInteger());
}

static auto _the_maximum_value_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _the_minimum_value_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static auto _the_average_of_T(CallFrame &frame, SourceLocation location, Value *values)
    -> Result<Value, Error> {
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
}

static void _core(ModuleMap &natives) {
    natives[S("the language version")] = N(_the_language_version);
    natives[S("the language major version")] = N(_the_language_major_version);
    natives[S("the language minor version")] = N(_the_language_minor_version);
    natives[S("the language patch version")] = N(_the_language_patch_version);
    natives[S("the error")] = N(_the_error);
    natives[S("error with {}")] = N(_error_with_T);
    natives[S("quit")] = N(_quit);
    natives[S("quit with {}")] = N(_quit_with_T);
    natives[S("get {}")] = N(_get_T);
    natives[S("(the) description of {}")] = N(_the_description_of_T);
    natives[S("(the) debug description of {}")] = N(_the_debug_description_of_T);
    natives[S("(the) hash value of {}")] = N(_the_hash_value_of_T);
    natives[S("(the) type name of {}")] = N(_the_type_name_of_T);
    natives[S("(a) copy of {}")] = N(_a_copy_of_T);
}

static void _common(ModuleMap &natives) {
    natives[S("(the) size of {}")] = N(_the_size_of_T);
    natives[S("{} contains {}")] = N(_T_contains_T);
    natives[S("{} is in {}")] = N(_T_is_in_T);
    natives[S("{} starts with {}")] = N(_T_starts_with_T);
    natives[S("{} ends with {}")] = N(_T_ends_with_T);
    natives[S("item {} in {}")] = N(_item_T_in_T);
    natives[S("insert {} at (the) end of {}")] = N(_insert_T_at_the_end_of_T);
    natives[S("remove item {} from {}")] = N(_remove_item_T_from_T);
    natives[S("(the) (first) offset of {} in {}")] = N(_the_first_offset_of_T_in_T);
    natives[S("(the) last offset of {} in {}")] = N(_the_last_offset_of_T_in_T);
    natives[S("replace all {} with {} in {}")] = N(_replace_all_T_with_T_in_T);
    natives[S("replace first {} with {} in {}")] = N(_replace_first_T_with_T_in_T);
    natives[S("replace last {} with {} in {}")] = N(_replace_last_T_with_T_in_T);
    natives[S("sort {}")] = N(_sort_T);
}

static void _types(ModuleMap &natives) {
    natives[S("{} as (a/an) integer")] = N(_T_as_an_integer);
    natives[S("{} as (a/an) number")] = N(_T_as_a_number);
    natives[S("{} as (a/an) string")] = N(_T_as_a_string);
}

static void _dictionary(ModuleMap &natives) {
    natives[S("(the) keys (of) {}")] = N(_the_keys_of_T);
    natives[S("(the) values (of) {}")] = N(_the_values_of_T);
    natives[S("insert item {} with key {} into {}")] = N(_insert_item_T_with_key_T_into_T);
}

static void _list(ModuleMap &natives, std::mt19937_64 &engine,
                  std::function<Integer(Integer)> randomInteger) {
    natives[S("items {} to {} (in/of) {}")] = N(_items_T_to_T_in_T);
    natives[S("(the) mid/middle item (in/of) {}")] = N(_the_middle_item_in_T);
    natives[S("(the) last item (in/of) {}")] = N(_the_last_item_in_T);
    natives[S("(the) number of items (in/of) {}")] = N(_the_number_of_items_in_T);
    natives[S("remove items {} to {} from {}")] = N(_remove_items_T_to_T_from_T);
    natives[S("insert {} at index {} into {}")] = N(_insert_T_at_index_T_into_T);
    natives[S("reverse {}")] = N(_reverse_T);
    natives[S("reversed {}")] = N(_reversed_T);
    natives[S("join {}")] = N(_join_T);
    natives[S("join {} using {}")] = N(_join_T_using_T);
    natives[S("any item (in/of) {}")] = N(_any_item_in_T(randomInteger));
    natives[S("shuffle {}")] = N(_shuffle_T(engine));
    natives[S("shuffled {}")] = N(_shuffled_T(engine));
}

static void _string(ModuleMap &natives, std::mt19937_64 &engine,
                    std::function<Integer(Integer)> randomInteger) {
    natives[S("insert {} at char/character {} in {}")] = N(_insert_T_at_character_T_in_T);
    natives[S("remove all {} from {}")] = N(_remove_all_T_from_T);
    natives[S("remove first {} from {}")] = N(_remove_first_T_from_T);
    natives[S("remove last {} from {}")] = N(_remove_last_T_from_T);

    natives[S("replace char/character {} with {} in {}")] =
        N(_replace_chunk_T_with_T_in_T(chunk::character));
    natives[S("replace word {} with {} in {}")] = N(_replace_chunk_T_with_T_in_T(chunk::word));
    natives[S("replace line {} with {} in {}")] = N(_replace_chunk_T_with_T_in_T(chunk::line));

    natives[S("replace chars/characters {} to {} with {} in {}")] =
        N(_replace_chunks_T_to_T_with_T_in_T(chunk::character));
    natives[S("replace words {} to {} with {} in {}")] =
        N(_replace_chunks_T_to_T_with_T_in_T(chunk::word));
    natives[S("replace lines {} to {} with {} in {}")] =
        N(_replace_chunks_T_to_T_with_T_in_T(chunk::line));

    natives[S("remove char/character {} from {}")] = N(_remove_chunk_T_from_T(chunk::character));
    natives[S("remove word {} from {}")] = N(_remove_chunk_T_from_T(chunk::word));
    natives[S("remove line {} from {}")] = N(_remove_chunk_T_from_T(chunk::line));

    natives[S("remove chars/characters {} to {} from {}")] =
        N(_remove_chunks_T_to_T_from_T(chunk::character));
    natives[S("remove words {} to {} from {}")] = N(_remove_chunks_T_to_T_from_T(chunk::word));
    natives[S("remove lines {} to {} from {}")] = N(_remove_chunks_T_to_T_from_T(chunk::line));

    natives[S("(the) list of chars/characters (in/of) {}")] =
        N(_the_list_of_chunks_in_T(chunk::character));
    natives[S("(the) list of words (in/of) {}")] = N(_the_list_of_chunks_in_T(chunk::word));
    natives[S("(the) list of lines (in/of) {}")] = N(_the_list_of_chunks_in_T(chunk::line));

    natives[S("char/character {} in/of {}")] = N(_chunk_T_in_T(chunk::character));
    natives[S("word {} in/of {}")] = N(_chunk_T_in_T(chunk::word));
    natives[S("line {} in/of {}")] = N(_chunk_T_in_T(chunk::line));

    natives[S("chars/characters {} to {} in/of {}")] = N(_chunks_T_to_T_in_T(chunk::character));
    natives[S("words {} to {} in/of {}")] = N(_chunks_T_to_T_in_T(chunk::word));
    natives[S("lines {} to {} in/of {}")] = N(_chunks_T_to_T_in_T(chunk::line));

    natives[S("any char/character in/of {}")] = N(_any_chunk_in_T(chunk::character, randomInteger));
    natives[S("any word in/of {}")] = N(_any_chunk_in_T(chunk::word, randomInteger));
    natives[S("any line in/of {}")] = N(_any_chunk_in_T(chunk::line, randomInteger));

    natives[S("(the) mid/middle char/character in/of {}")] =
        N(_the_middle_chunk_in_T(chunk::character));
    natives[S("(the) mid/middle word in/of {}")] = N(_the_middle_chunk_in_T(chunk::word));
    natives[S("(the) mid/middle line in/of {}")] = N(_the_middle_chunk_in_T(chunk::line));

    natives[S("(the) last char/character in/of {}")] = N(_the_last_chunk_in_T(chunk::character));
    natives[S("(the) last word in/of {}")] = N(_the_last_chunk_in_T(chunk::word));
    natives[S("(the) last line in/of {}")] = N(_the_last_chunk_in_T(chunk::line));

    natives[S("(the) number of chars/characters (in/of) {}")] =
        N(_the_number_of_chunks_in_T(chunk::character));
    natives[S("(the) number of words (in/of) {}")] = N(_the_number_of_chunks_in_T(chunk::word));
    natives[S("(the) number of lines (in/of) {}")] = N(_the_number_of_chunks_in_T(chunk::line));
}

static void _range(ModuleMap &natives, std::mt19937_64 &engine,
                   std::function<Integer(Integer)> randomInteger) {
    natives[S("{} up to {}")] = N(_T_up_to_T);
    natives[S("(the) lower bound (in/of) {}")] = N(_the_lower_bound_of_T);
    natives[S("(the) upper bound (in/of) {}")] = N(_the_upper_bound_of_T);
    natives[S("{} is closed")] = N(_T_is_closed);
    natives[S("{} overlaps (with) {}")] = N(_T_overlaps_with_T);
    natives[S("(a) random number (in/of) {}")] = N(_a_random_number_in_T(randomInteger));
}

static void _math(ModuleMap &natives) {
    natives[S("(the) abs (of) {}")] = N(_the_abs_of_T);
    natives[S("(the) sin (of) {}")] = N(_the_func_of_T(sin));
    natives[S("(the) cos (of) {}")] = N(_the_func_of_T(cos));
    natives[S("(the) tan (of) {}")] = N(_the_func_of_T(tan));
    natives[S("(the) atan (of) {}")] = N(_the_func_of_T(atan));
    natives[S("(the) exp (of) {}")] = N(_the_func_of_T(exp));
    natives[S("(the) exp2 (of) {}")] = N(_the_func_of_T(exp2));
    natives[S("(the) expm1 (of) {}")] = N(_the_func_of_T(expm1));
    natives[S("(the) log2 (of) {}")] = N(_the_func_of_T(log2));
    natives[S("(the) log10 (of) {}")] = N(_the_func_of_T(log10));
    natives[S("(the) log (of) {}")] = N(_the_func_of_T(log));
    natives[S("(the) sqrt (of) {}")] = N(_the_func_of_T(sqrt));
    natives[S("(the) ceil (of) {}")] = N(_the_func_of_T(ceil));
    natives[S("(the) floor (of) {}")] = N(_the_func_of_T(floor));
    natives[S("round {}")] = N(_the_func_of_T(round));
    natives[S("trunc/truncate {}")] = N(_the_func_of_T(trunc));

    natives[S("(the) max/maximum (value) (of) {}")] = N(_the_maximum_value_of_T);
    natives[S("(the) min/minimum (value) (of) {}")] = N(_the_minimum_value_of_T);
    natives[S("(the) avg/average (value) (of) {}")] = N(_the_average_of_T);
}

Core::Core(const CoreConfig &config) : _config(config) {
    _core(_natives);
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
