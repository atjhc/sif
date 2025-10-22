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

#include "Utilities.h"
#include "sif/Error.h"

#include "sif/runtime/modules/Core.h"
#include "sif/runtime/objects/Dictionary.h"
#include "sif/runtime/objects/List.h"
#include "sif/runtime/objects/Native.h"
#include "sif/runtime/objects/String.h"

#include "extern/utf8.h"
#include "utilities/chunk.h"
#include "utilities/strings.h"

#include <charconv>
#include <format>
#include <random>
#include <utility>

SIF_NAMESPACE_BEGIN

#define N(X) MakeStrong<Native>(X)

template <typename F>
std::function<Result<Value, Error>(const NativeCallContext &)> adaptLegacyFunction(F func) {
    return [func](const NativeCallContext &context) -> Result<Value, Error> {
        return func(context.vm, context.location, context.arguments);
    };
}

using ModuleMap = Mapping<Signature, Strong<Native>, Signature::Hash>;
static Signature S(const char *signature) { return Signature::Make(signature).value(); }

namespace Errors {
inline constexpr std::string_view CantCompare = "can't compare “{}” ({}) and “{}” ({})";
inline constexpr std::string_view CantConvertToInteger = "can't convert this value to an integer";
inline constexpr std::string_view CantConvertToNumber = "can't convert this value to a number";
inline constexpr std::string_view DomainError = "domain error";
inline constexpr std::string_view ExpectedADictionary = "expected a dictionary";
inline constexpr std::string_view ExpectedADictionaryOrList = "expected a dictionary or list";
inline constexpr std::string_view ExpectedAList = "expected a list";
inline constexpr std::string_view ExpectedAnInteger = "expected an integer";
inline constexpr std::string_view ExpectedANumber = "expected a number";
inline constexpr std::string_view ExpectedARange = "expected a range";
inline constexpr std::string_view ExpectedIntegerOrRange = "expected an integer or range";
inline constexpr std::string_view ExpectedListOrDictionary = "expected a list or dictionary";
inline constexpr std::string_view ExpectedStringOrList = "expected a string or list";
inline constexpr std::string_view FormatOutOfRange = "format index out of range";
inline constexpr std::string_view InvalidFormatIndex = "invalid format index";
inline constexpr std::string_view InvalidUnicodeCodePoint = "invalid unicode codepoint";
inline constexpr std::string_view ListIsEmpty = "list is empty";
inline constexpr std::string_view NotEnoughFormatArgs = "not enough arguments for format";
inline constexpr std::string_view UnterminatedFormat = "unterminated placeholder in format string";
} // namespace Errors

static auto _the_language_version(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(std::string(Version));
}

static auto _the_language_major_version(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(MajorVersion);
}

static auto _the_language_minor_version(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(MinorVersion);
}

static auto _the_language_patch_version(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(PatchVersion);
}

static auto _the_error(const NativeCallContext &context) -> Result<Value, Error> {
    return context.vm.error();
}

static auto _error_with_T(const NativeCallContext &context) -> Result<Value, Error> {
    return Fail(Error(context.location, context.arguments[0]));
}

static auto _quit(const NativeCallContext &context) -> Result<Value, Error> { exit(0); }

static auto _quit_with_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (!context.arguments[0].isInteger()) {
        return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
    }
    exit(static_cast<int>(context.arguments[0].asInteger()));
}

static auto _get_T(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0];
}

static auto _the_description_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    std::ostringstream ss;
    ss << context.arguments[0];
    return ss.str();
}

static auto _the_debug_description_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].debugDescription();
}

static auto _the_hash_value_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    return Integer(Value::Hash()(context.arguments[0]));
}

static auto _the_type_name_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    return lowercase(context.arguments[0].typeName());
}

static auto _a_copy_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto copyable = context.arguments[0].as<Copyable>()) {
        return copyable->copy(context.vm);
    }
    return context.arguments[0];
}

static bool _case_insensitive_lexicographic_compare(const std::string &a, const std::string &b) {
    auto itA = a.begin();
    auto itB = b.begin();
    while (itA != a.end() && itB != b.end()) {
        char lowerA = std::tolower(*itA);
        char lowerB = std::tolower(*itB);
        if (lowerA < lowerB)
            return true;
        if (lowerA > lowerB)
            return false;
        ++itA;
        ++itB;
    }
    return a.size() < b.size();
}

static auto _sort_list(const NativeCallContext &context, Strong<List> list)
    -> Result<Value, Error> {
    try {
        std::sort(
            list->values().begin(), list->values().end(), [&](const Value &a, const Value &b) {
                if (a.isInteger() && b.isInteger()) {
                    return a.asInteger() < b.asInteger();
                } else if (a.isNumber() && b.isNumber()) {
                    return a.castFloat() < b.castFloat();
                } else if (a.isString() && b.isString()) {
                    return _case_insensitive_lexicographic_compare(a.toString(), b.toString());
                }
                throw Error(context.location, Errors::CantCompare, a.toString(), a.typeName(),
                            b.toString(), b.typeName());
                return true;
            });
    } catch (const Error &error) {
        return Fail(error);
    }
    return Value(list);
}

static auto _sort_string(const NativeCallContext &context, Strong<String> string)
    -> Result<Value, Error> {
    return Fail(Error(context.location, Errors::ExpectedAList));
}

static auto _sort_dictionary(const NativeCallContext &context, Strong<Dictionary> dictionary)
    -> Result<Value, Error> {
    return Fail(Error(context.location, Errors::ExpectedAList));
}

static auto _sort_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto list = context.arguments[0].as<List>()) {
        return _sort_list(context, list);
    } else if (auto string = context.arguments[0].as<String>()) {
        return _sort_string(context, string);
    } else if (auto dictionary = context.arguments[0].as<Dictionary>()) {
        return _sort_dictionary(context, dictionary);
    }
    return context.arguments[0];
}

static auto _the_size_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    size_t size = 0;
    if (auto list = context.arguments[0].as<List>()) {
        size = list->values().size();
    } else if (auto dictionary = context.arguments[0].as<Dictionary>()) {
        size = dictionary->values().size();
    } else if (auto string = context.arguments[0].as<String>()) {
        size = string->string().size();
    } else if (auto range = context.arguments[0].as<Range>()) {
        size = range->size();
    } else {
        return Fail(context.argumentError(0, Errors::ExpectedListStringDictRange));
    }
    return static_cast<long>(size);
}

static auto _T_is_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (context.arguments[1].isEmpty()) {
        if (auto list = context.arguments[0].as<List>()) {
            return list->values().size() == 0;
        } else if (auto dictionary = context.arguments[0].as<Dictionary>()) {
            return dictionary->values().size() == 0;
        } else if (auto string = context.arguments[0].as<String>()) {
            return string->string().size() == 0;
        } else if (auto range = context.arguments[0].as<Range>()) {
            return range->size() == 0;
        }
    }
    return context.arguments[0] == context.arguments[1];
}

static auto _T_is_not_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (context.arguments[1].isEmpty()) {
        if (auto list = context.arguments[0].as<List>()) {
            return list->values().size() != 0;
        } else if (auto dictionary = context.arguments[0].as<Dictionary>()) {
            return dictionary->values().size() != 0;
        } else if (auto string = context.arguments[0].as<String>()) {
            return string->string().size() != 0;
        } else if (auto range = context.arguments[0].as<Range>()) {
            return range->size() != 0;
        }
    }
    return context.arguments[0] != context.arguments[1];
}

static auto _contains(const NativeCallContext &context, int containerIndex, int valueIndex)
    -> Result<Value, Error> {
    auto object = context.arguments[containerIndex];
    auto value = context.arguments[valueIndex];
    if (auto list = object.as<List>()) {
        return list->contains(value);
    } else if (auto dictionary = object.as<Dictionary>()) {
        return dictionary->contains(value);
    } else if (auto string = object.as<String>()) {
        if (auto lookup = value.as<String>()) {
            return string->string().find(lookup->string()) != std::string::npos;
        }
        return Fail(context.argumentError(valueIndex, Errors::ExpectedAString));
    } else if (auto range = object.as<Range>()) {
        if (auto queryRange = value.as<Range>()) {
            return range->contains(*queryRange);
        }
        if (!value.isInteger()) {
            return Fail(context.argumentError(valueIndex, Errors::ExpectedIntegerOrRange));
        }
        return range->contains(value.asInteger());
    }
    return Fail(context.argumentError(containerIndex, Errors::ExpectedListStringDictRange));
}

static auto _T_contains_T(const NativeCallContext &context) -> Result<Value, Error> {
    return _contains(context, 0, 1);
}

static auto _T_is_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    return _contains(context, 1, 0);
}

static auto _T_starts_with_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto string = context.arguments[0].as<String>()) {
        auto searchString = context.arguments[1].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        return string->startsWith(*searchString);
    } else if (auto list = context.arguments[0].as<List>()) {
        return list->startsWith(context.arguments[1]);
    }
    return Fail(context.argumentError(0, Errors::ExpectedStringOrList));
}

static auto _T_ends_with_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto string = context.arguments[0].as<String>()) {
        auto searchString = context.arguments[1].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        return string->endsWith(*searchString);
    } else if (auto list = context.arguments[0].as<List>()) {
        return list->endsWith(context.arguments[1]);
    }
    return Fail(context.argumentError(0, Errors::ExpectedStringOrList));
}

static auto _item_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto list = context.arguments[1].as<List>()) {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        return list->subscript(context.vm, context.location, context.arguments[0]);
    } else if (auto dictionary = context.arguments[1].as<Dictionary>()) {
        return dictionary->subscript(context.vm, context.location, context.arguments[0]);
    }
    return Fail(context.argumentError(1, Errors::ExpectedListOrDictionary));
}

static auto _insert_T_at_the_beginning_of_T(const NativeCallContext &context)
    -> Result<Value, Error> {
    if (auto list = context.arguments[1].as<List>()) {
        list->values().insert(list->values().begin(), context.arguments[0]);
        context.vm.notifyContainerMutation(list.get());
    } else if (auto string = context.arguments[1].as<String>()) {
        auto insertText = context.arguments[0].as<String>();
        if (!insertText) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        string->string().insert(0, insertText->string());
    } else {
        return Fail(context.argumentError(1, Errors::ExpectedStringOrList));
    }
    return context.arguments[1];
}

static auto _insert_T_at_the_end_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto list = context.arguments[1].as<List>()) {
        list->values().push_back(context.arguments[0]);
        context.vm.notifyContainerMutation(list.get());
    } else if (auto string = context.arguments[1].as<String>()) {
        auto insertText = context.arguments[0].as<String>();
        if (!insertText) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        string->string().append(insertText->string());
    } else {
        return Fail(context.argumentError(1, Errors::ExpectedStringOrList));
    }
    return context.arguments[1];
}

static auto _remove_the_first_item_from_T(const NativeCallContext &context)
    -> Result<Value, Error> {
    if (auto list = context.arguments[0].as<List>()) {
        if (list->size() == 0) {
            return Value();
        }
        list->values().erase(list->values().begin());
        context.vm.notifyContainerMutation(list.get());
        return list;
    }
    return Fail(context.argumentError(0, Errors::ExpectedAList));
}

static auto _remove_the_last_item_from_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto list = context.arguments[0].as<List>()) {
        if (list->size() == 0) {
            return Value();
        }
        list->values().pop_back();
        context.vm.notifyContainerMutation(list.get());
        return list;
    }
    return Fail(context.argumentError(0, Errors::ExpectedAList));
}

static auto _remove_item_T_from_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto dictionary = context.arguments[1].as<Dictionary>()) {
        dictionary->values().erase(context.arguments[0]);
        context.vm.notifyContainerMutation(dictionary.get());
        return dictionary;
    } else if (auto list = context.arguments[1].as<List>()) {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        auto index = context.arguments[0].asInteger();
        list->values().erase(list->values().begin() + index);
        context.vm.notifyContainerMutation(list.get());
        return list;
    }
    return Fail(context.argumentError(1, Errors::ExpectedADictionaryOrList));
}

static auto _the_first_offset_of_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto text = context.arguments[1].as<String>()) {
        auto searchString = context.arguments[0].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        auto result = text->findFirst(*searchString);
        return result == std::string::npos ? Value() : Integer(result);
    } else if (auto list = context.arguments[1].as<List>()) {
        if (auto result = list->findFirst(context.arguments[0])) {
            return result.value();
        }
        return Value();
    }
    return Fail(context.argumentError(1, Errors::ExpectedAString));
}

static auto _the_last_offset_of_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto text = context.arguments[1].as<String>()) {
        auto searchString = context.arguments[0].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(0, Errors::ExpectedStringOrList));
        }
        auto result = text->findLast(*searchString);
        return result == std::string::npos ? Value() : Integer(result);
    } else if (auto list = context.arguments[1].as<List>()) {
        if (auto result = list->findLast(context.arguments[0])) {
            return result.value();
        }
        return Value();
    }
    return Fail(context.argumentError(1, Errors::ExpectedStringOrList));
}

static auto _replace_all_T_with_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto text = context.arguments[2].as<String>()) {
        auto searchString = context.arguments[0].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        auto replacementString = context.arguments[1].as<String>();
        if (!replacementString) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        text->replaceAll(*searchString, *replacementString);
        return text;
    } else if (auto list = context.arguments[2].as<List>()) {
        list->replaceAll(context.arguments[0], context.arguments[1]);
        context.vm.notifyContainerMutation(list.get());
        return list;
    }
    return Fail(context.argumentError(2, Errors::ExpectedStringOrList));
}

static auto _replace_first_T_with_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto text = context.arguments[2].as<String>()) {
        auto searchString = context.arguments[0].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        auto replacementString = context.arguments[1].as<String>();
        if (!replacementString) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        text->replaceFirst(*searchString, *replacementString);
        return text;
    } else if (auto list = context.arguments[2].as<List>()) {
        list->replaceFirst(context.arguments[0], context.arguments[1]);
        context.vm.notifyContainerMutation(list.get());
        return list;
    }
    return Fail(context.argumentError(2, Errors::ExpectedStringOrList));
}

static auto _replace_last_T_with_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto text = context.arguments[2].as<String>()) {
        auto searchString = context.arguments[0].as<String>();
        if (!searchString) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        auto replacementString = context.arguments[1].as<String>();
        if (!replacementString) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        text->replaceLast(*searchString, *replacementString);
        return text;
    } else if (auto list = context.arguments[2].as<List>()) {
        list->replaceLast(context.arguments[0], context.arguments[1]);
        context.vm.notifyContainerMutation(list.get());
        return list;
    }
    return Fail(context.argumentError(2, Errors::ExpectedStringOrList));
}

static auto _T_as_an_integer(const NativeCallContext &context) -> Result<Value, Error> {
    if (context.arguments[0].isNumber()) {
        return Value(context.arguments[0].castInteger());
    }
    if (auto castable = context.arguments[0].as<NumberCastable>()) {
        return castable->castInteger();
    }
    return Fail(context.argumentError(0, Errors::CantConvertToInteger));
}

static auto _T_as_a_number(const NativeCallContext &context) -> Result<Value, Error> {
    if (context.arguments[0].isNumber()) {
        return Value(context.arguments[0].castFloat());
    }
    if (auto castable = context.arguments[0].as<NumberCastable>()) {
        return castable->castFloat();
    }
    return Fail(context.argumentError(0, Errors::CantConvertToNumber));
}

static auto _T_as_a_string(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].toString();
}

static auto _T_is_a_integer(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].isInteger();
}

static auto _T_is_a_number(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].isNumber();
}

static auto _T_is_a_string(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].isString();
}

static auto _T_is_a_list(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].as<List>() != nullptr;
}

static auto _T_is_a_dictionary(const NativeCallContext &context) -> Result<Value, Error> {
    return context.arguments[0].as<Dictionary>() != nullptr;
}

static auto _an_empty_string(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(std::string());
}

static auto _an_empty_list(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(context.vm.make<List>());
}

static auto _an_empty_dictionary(const NativeCallContext &context) -> Result<Value, Error> {
    return Value(context.vm.make<Dictionary>());
}

static auto _the_keys_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto dictionary = context.arguments[0].as<Dictionary>();
    if (!dictionary) {
        return Fail(context.argumentError(0, Errors::ExpectedADictionary));
    }
    std::vector<Value> keys;
    keys.reserve(dictionary->values().size());
    for (const auto &pair : dictionary->values()) {
        keys.emplace_back(pair.first);
    }
    return context.vm.make<List>(std::move(keys));
}

static auto _the_values_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto dictionary = context.arguments[0].as<Dictionary>();
    if (!dictionary) {
        return Fail(context.argumentError(0, Errors::ExpectedADictionary));
    }
    std::vector<Value> values;
    values.reserve(dictionary->values().size());
    for (const auto &pair : dictionary->values()) {
        values.emplace_back(pair.second);
    }
    return context.vm.make<List>(std::move(values));
}

static auto _insert_item_T_with_key_T_into_T(const NativeCallContext &context)
    -> Result<Value, Error> {
    auto dictionary = context.arguments[2].as<Dictionary>();
    if (!dictionary) {
        return Fail(context.argumentError(2, Errors::ExpectedADictionary));
    }
    dictionary->values()[context.arguments[1]] = context.arguments[0];
    context.vm.notifyContainerMutation(dictionary.get());
    return dictionary;
}

static auto _items_T_to_T_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[2].as<List>();
    if (!list) {
        return Fail(context.argumentError(2, Errors::ExpectedAList));
    }
    if (!context.arguments[0].isInteger()) {
        return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
    }
    if (!context.arguments[1].isInteger()) {
        return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
    }
    auto index1 = context.arguments[0].asInteger();
    auto index2 = context.arguments[1].asInteger();
    auto result =
        context.vm.make<List>(list->values().begin() + index1, list->values().begin() + index2 + 1);
    context.vm.notifyContainerMutation(result.get());
    return result;
}

static auto _the_first_item_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    if (list->size() == 0) {
        return Value();
    }
    return list->values().at(0);
}

static auto _the_middle_item_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    if (list->size() == 0) {
        return Value();
    }
    return list->values().at(list->values().size() / 2);
}

static auto _the_last_item_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    if (list->size() == 0) {
        return Value();
    }
    return list->values().back();
}

static auto _the_number_of_items_in_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    return Integer(list->values().size());
}

static auto _any_item_in_T(std::function<Integer(Integer)> randomInteger)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [randomInteger](const NativeCallContext &context) -> Result<Value, Error> {
        auto list = context.arguments[0].as<List>();
        if (!list) {
            return Fail(context.argumentError(0, Errors::ExpectedAList));
        }
        return list->values().at(randomInteger(list->values().size()));
    };
}

static auto _remove_items_T_to_T_from_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[2].as<List>();
    if (!list) {
        return Fail(context.argumentError(2, Errors::ExpectedAList));
    }
    if (!context.arguments[0].isInteger()) {
        return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
    }
    if (!context.arguments[1].isInteger()) {
        return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
    }
    auto index1 = context.arguments[0].asInteger();
    auto index2 = context.arguments[1].asInteger();
    list->values().erase(list->values().begin() + index1, list->values().begin() + index2 + 1);
    context.vm.notifyContainerMutation(list.get());
    return list;
}

static auto _insert_T_at_index_T_into_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[2].as<List>();
    if (!list) {
        return Fail(context.argumentError(2, Errors::ExpectedAList));
    }
    if (!context.arguments[1].isInteger()) {
        return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
    }
    auto index = context.arguments[1].asInteger();
    list->values().insert(list->values().begin() + index, context.arguments[0]);
    context.vm.notifyContainerMutation(list.get());
    return list;
}

static auto _reverse_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    std::reverse(list->values().begin(), list->values().end());
    return list;
}

static auto _reversed_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    return context.vm.make<List>(list->values().rbegin(), list->values().rend());
}

static auto _shuffle_T(std::mt19937_64 &engine)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [&engine](const NativeCallContext &context) -> Result<Value, Error> {
        auto list = context.arguments[0].as<List>();
        if (!list) {
            return Fail(context.argumentError(0, Errors::ExpectedAList));
        }
        std::shuffle(list->values().begin(), list->values().end(), engine);
        return list;
    };
}

static auto _shuffled_T(std::mt19937_64 &engine)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [&engine](const NativeCallContext &context) -> Result<Value, Error> {
        auto list = context.arguments[0].as<List>();
        if (!list) {
            return Fail(context.argumentError(0, Errors::ExpectedAList));
        }
        auto result = context.vm.make<List>(list->values());
        std::shuffle(result->values().begin(), result->values().end(), engine);
        return result;
    };
}

static auto _join_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    std::ostringstream str;
    for (auto it = list->values().begin(); it < list->values().end(); it++) {
        str << it->toString();
    }
    return str.str();
}

static auto _join_T_using_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    auto joinString = context.arguments[1].as<String>();
    if (!joinString) {
        return Fail(context.argumentError(1, Errors::ExpectedAString));
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

static auto _insert_T_at_character_T_in_T(const NativeCallContext &context)
    -> Result<Value, Error> {
    auto insertText = context.arguments[0].as<String>();
    if (!insertText) {
        return Fail(context.argumentError(0, Errors::ExpectedAString));
    }
    if (!context.arguments[1].isInteger()) {
        return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
    }
    auto text = context.arguments[2].as<String>();
    if (!text) {
        return Fail(context.argumentError(2, Errors::ExpectedAString));
    }
    auto chunk =
        index_chunk(chunk::type::character, context.arguments[1].asInteger(), text->string());
    text->string().insert(chunk.begin(), insertText->string().begin(), insertText->string().end());
    return text;
}

static auto _remove_all_T_from_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto removeText = context.arguments[0].as<String>();
    if (!removeText) {
        return Fail(context.argumentError(0, Errors::ExpectedAString));
    }
    auto text = context.arguments[1].as<String>();
    if (!text) {
        return Fail(context.argumentError(1, Errors::ExpectedAString));
    }
    text->replaceAll(*removeText, String(""));
    return text;
}

static auto _remove_first_T_from_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto removeText = context.arguments[0].as<String>();
    if (!removeText) {
        return Fail(context.argumentError(0, Errors::ExpectedAString));
    }
    auto text = context.arguments[1].as<String>();
    if (!text) {
        return Fail(context.argumentError(1, Errors::ExpectedAString));
    }
    text->replaceFirst(*removeText, String(""));
    return text;
}

static auto _remove_last_T_from_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto removeText = context.arguments[0].as<String>();
    if (!removeText) {
        return Fail(context.argumentError(0, Errors::ExpectedAString));
    }
    auto text = context.arguments[1].as<String>();
    if (!text) {
        return Fail(context.argumentError(1, Errors::ExpectedAString));
    }
    text->replaceLast(*removeText, String(""));
    return text;
}

static auto _replace_chunk_T_with_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        auto index = context.arguments[0].asInteger();
        auto replacement = context.arguments[1].as<String>();
        if (!replacement) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        auto text = context.arguments[2].as<String>();
        if (!text) {
            return Fail(context.argumentError(2, Errors::ExpectedAString));
        }

        auto chunk = index_chunk(chunkType, index, text->string());
        text->string().replace(chunk.begin(), chunk.end(), replacement->string());
        return text;
    };
}

static auto _replace_chunks_T_to_T_with_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isInteger() || !context.arguments[1].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        auto start = context.arguments[0].asInteger();
        auto end = context.arguments[1].asInteger();
        auto replacement = context.arguments[2].as<String>();
        if (!replacement) {
            return Fail(context.argumentError(2, Errors::ExpectedAString));
        }
        auto text = context.arguments[3].as<String>();
        if (!text) {
            return Fail(context.argumentError(3, Errors::ExpectedAString));
        }
        auto chunk = range_chunk(chunkType, start, end, text->string());
        text->string().replace(chunk.begin(), chunk.end(), replacement->string());
        return text;
    };
}

static auto _remove_chunk_T_from_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        auto index = context.arguments[0].asInteger();
        auto text = context.arguments[1].as<String>();
        if (!text) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        auto chunk = index_chunk(chunkType, index, text->string());
        text->string().erase(chunk.begin(), chunk.end());
        return text;
    };
}

static auto _remove_chunks_T_to_T_from_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        if (!context.arguments[1].isInteger()) {
            return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
        }
        auto start = context.arguments[0].asInteger();
        auto end = context.arguments[1].asInteger();
        auto text = context.arguments[2].as<String>();
        if (!text) {
            return Fail(context.argumentError(2, Errors::ExpectedAString));
        }
        auto chunk = range_chunk(chunkType, start, end, text->string());
        text->string().erase(chunk.begin(), chunk.end());
        return text;
    };
}

static auto _the_list_of_chunks_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        auto text = context.arguments[0].as<String>();
        if (!text) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        std::vector<Value> result;
        size_t index = 0;
        auto chunk = index_chunk(chunkType, index++, text->string());
        while (chunk.begin() < text->string().end()) {
            result.push_back(Value(chunk.get()));
            chunk = index_chunk(chunkType, index++, text->string());
        }
        return context.vm.make<List>(std::move(result));
    };
}

static auto _chunk_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        auto index = context.arguments[0].asInteger();
        auto text = context.arguments[1].as<String>();
        if (!text) {
            return Fail(context.argumentError(1, Errors::ExpectedAString));
        }
        return index_chunk(chunkType, index, text->string()).get();
    };
}

static auto _chunks_T_to_T_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isInteger()) {
            return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
        }
        if (!context.arguments[1].isInteger()) {
            return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
        }
        auto start = context.arguments[0].asInteger();
        auto end = context.arguments[1].asInteger();
        auto text = context.arguments[2].as<String>();
        if (!text) {
            return Fail(context.argumentError(2, Errors::ExpectedAString));
        }
        return range_chunk(chunkType, start, end, text->string()).get();
    };
}

static auto _any_chunk_in_T(chunk::type chunkType, std::function<Integer(Integer)> randomInteger)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType, randomInteger](const NativeCallContext &context) -> Result<Value, Error> {
        auto text = context.arguments[0].as<String>();
        if (!text) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        return random_chunk(chunkType, randomInteger, text->string()).get();
    };
}

static auto _the_middle_chunk_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        auto text = context.arguments[0].as<String>();
        if (!text) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        return middle_chunk(chunkType, text->string()).get();
    };
}

static auto _the_last_chunk_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        auto text = context.arguments[0].as<String>();
        if (!text) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        return last_chunk(chunkType, text->string()).get();
    };
}

static auto _the_number_of_chunks_in_T(chunk::type chunkType)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [chunkType](const NativeCallContext &context) -> Result<Value, Error> {
        auto text = context.arguments[0].as<String>();
        if (!text) {
            return Fail(context.argumentError(0, Errors::ExpectedAString));
        }
        return static_cast<Integer>(count_chunk(chunkType, text->string()).count);
    };
}

static auto _format_string_T_with_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (!context.arguments[0].isString()) {
        return Fail(context.argumentError(0, Errors::ExpectedAString));
    }
    auto formatStr = context.arguments[0].as<String>()->string();

    std::vector<Value> args;
    if (auto list = context.arguments[1].as<List>()) {
        args = list->values();
    } else {
        args.push_back(context.arguments[1]);
    }

    std::ostringstream result;
    size_t pos = 0;
    size_t nextPos = 0;
    size_t argIndex = 0;

    while ((nextPos = formatStr.find('{', pos)) != std::string::npos) {
        if (nextPos > 0 && formatStr[nextPos - 1] == '\\') {
            result << formatStr.substr(pos, nextPos - pos - 1) << '{';
            pos = nextPos + 1;
            continue;
        }

        result << formatStr.substr(pos, nextPos - pos);
        size_t closeBrace = formatStr.find('}', nextPos);
        if (closeBrace == std::string::npos) {
            return Fail(context.argumentError(0, Errors::UnterminatedFormat));
        }

        if (closeBrace > nextPos + 1) {
            std::string indexStr = formatStr.substr(nextPos + 1, closeBrace - nextPos - 1);
            try {
                size_t index = std::stoul(indexStr);
                if (index >= args.size()) {
                    return Fail(context.argumentError(0, Errors::FormatOutOfRange));
                }
                result << args[index];
            } catch (std::exception &e) {
                return Fail(context.argumentError(0, Errors::InvalidFormatIndex));
            }
        } else {
            if (argIndex >= args.size()) {
                return Fail(context.argumentError(0, Errors::NotEnoughFormatArgs));
            }
            result << args[argIndex++];
        }
        pos = closeBrace + 1;
    }

    result << formatStr.substr(pos);
    return result.str();
}

static auto _character_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (!context.arguments[0].isInteger()) {
        return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
    }
    auto value = context.arguments[0].asInteger();
    try {
        return encode_utf8(static_cast<uint32_t>(value));
    } catch (...) {
        return Fail(context.argumentError(1, Errors::InvalidUnicodeCodePoint));
    }
}

static auto _ordinal_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (!context.arguments[0].isString()) {
        return Fail(context.argumentError(0, Errors::ExpectedAString));
    }
    try {
        return decode_utf8(context.arguments[0].toString());
    } catch (const std::exception &e) {
        return Fail(context.argumentError(0, "{}", e.what()));
    }
}

static auto _T_up_to_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (!context.arguments[0].isInteger()) {
        return Fail(context.argumentError(0, Errors::ExpectedAnInteger));
    }
    if (!context.arguments[1].isInteger()) {
        return Fail(context.argumentError(1, Errors::ExpectedAnInteger));
    }
    return MakeStrong<Range>(context.arguments[0].asInteger(), context.arguments[1].asInteger(),
                             true);
}

static auto _the_lower_bound_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto range = context.arguments[0].as<Range>()) {
        return range->start();
    }
    return Fail(context.argumentError(0, Errors::ExpectedARange));
}

static auto _the_upper_bound_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto range = context.arguments[0].as<Range>()) {
        return range->end();
    }
    return Fail(context.argumentError(0, Errors::ExpectedARange));
}

static auto _T_is_closed(const NativeCallContext &context) -> Result<Value, Error> {
    if (auto range = context.arguments[0].as<Range>()) {
        return range->closed();
    }
    return Fail(context.argumentError(0, Errors::ExpectedARange));
}

static auto _T_overlaps_with_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto range1 = context.arguments[0].as<Range>();
    auto range2 = context.arguments[1].as<Range>();
    if (!range1) {
        return Fail(context.argumentError(0, Errors::ExpectedARange));
    }
    if (!range2) {
        return Fail(context.argumentError(1, Errors::ExpectedARange));
    }
    return range1->overlaps(*range2);
}

static auto _a_random_number_in_T(std::function<Integer(Integer)> randomInteger)
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [randomInteger](const NativeCallContext &context) -> Result<Value, Error> {
        if (auto range = context.arguments[0].as<Range>()) {
            return range->start() +
                   randomInteger(range->end() - range->start() + (range->closed() ? 1 : 0));
        }
        return Fail(context.argumentError(0, Errors::ExpectedARange));
    };
}

static auto _the_func_of_T(double (*func)(double))
    -> std::function<Result<Value, Error>(const NativeCallContext &)> {
    return [func](const NativeCallContext &context) -> Result<Value, Error> {
        if (!context.arguments[0].isNumber()) {
            return Fail(context.argumentError(0, Errors::ExpectedANumber));
        }
        auto argument = context.arguments[0].castFloat();
        auto result = func(argument);
        if (isnan(result)) {
            return Fail(context.argumentError(0, Errors::DomainError));
        }
        return result;
    };
}

static auto _the_abs_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    if (!context.arguments[0].isNumber()) {
        return Fail(context.argumentError(0, Errors::ExpectedANumber));
    }
    if (context.arguments[0].isFloat()) {
        return std::fabs(context.arguments[0].asFloat());
    }
    return std::abs(context.arguments[0].asInteger());
}

static auto _the_maximum_value_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    if (list->values().size() == 0) {
        return Fail(context.argumentError(0, Errors::ListIsEmpty));
    }
    auto first = list->values().front();
    if (!first.isNumber()) {
        return Fail(context.argumentError(0, Errors::ExpectedANumber));
    }
    auto max = first.castFloat();
    for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
        if (!it->isNumber()) {
            return Fail(context.argumentError(0, Errors::ExpectedANumber));
        }
        auto value = it->castFloat();
        if (value > max) {
            max = value;
        }
    }
    return max;
}

static auto _the_minimum_value_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    if (list->values().size() == 0) {
        return Fail(context.argumentError(0, Errors::ListIsEmpty));
    }
    auto first = list->values().front();
    if (!first.isNumber()) {
        return Fail(context.argumentError(0, Errors::ExpectedANumber));
    }
    auto min = first.castFloat();
    for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
        if (!it->isNumber()) {
            return Fail(context.argumentError(0, Errors::ExpectedANumber));
        }
        auto value = it->castFloat();
        if (value < min) {
            min = value;
        }
    }
    return min;
}

static auto _the_average_of_T(const NativeCallContext &context) -> Result<Value, Error> {
    auto list = context.arguments[0].as<List>();
    if (!list) {
        return Fail(context.argumentError(0, Errors::ExpectedAList));
    }
    if (list->values().size() == 0) {
        return Fail(context.argumentError(0, Errors::ListIsEmpty));
    }
    auto first = list->values().front();
    if (!first.isNumber()) {
        return Fail(context.argumentError(0, Errors::ExpectedANumber));
    }
    auto sum = first.castFloat();
    for (auto it = list->values().begin() + 1; it < list->values().end(); it++) {
        if (!it->isNumber()) {
            return Fail(context.argumentError(0, Errors::ExpectedANumber));
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
    natives[S("(the) description (of) {}")] = N(_the_description_of_T);
    natives[S("(the) debug description (of) {}")] = N(_the_debug_description_of_T);
    natives[S("(the) hash value (of) {}")] = N(_the_hash_value_of_T);
    natives[S("(the) type name (of) {}")] = N(_the_type_name_of_T);
    natives[S("(a) copy (of) {}")] = N(_a_copy_of_T);
}

static void _common(ModuleMap &natives) {
    natives[S("(the) size of {}")] = N(_the_size_of_T);
    natives[S("{} is {}")] = N(_T_is_T);
    natives[S("{} is not {}")] = N(_T_is_not_T);
    natives[S("{} contains {}")] = N(_T_contains_T);
    natives[S("{} is in {}")] = N(_T_is_in_T);
    natives[S("{} starts with {}")] = N(_T_starts_with_T);
    natives[S("{} ends with {}")] = N(_T_ends_with_T);
    natives[S("item {} in {}")] = N(_item_T_in_T);
    natives[S("insert {} at (the) beginning of {}")] = N(_insert_T_at_the_beginning_of_T);
    natives[S("insert {} at (the) end of {}")] = N(_insert_T_at_the_end_of_T);
    natives[S("remove (the) first item from {}")] = N(_remove_the_first_item_from_T);
    natives[S("remove (the) last item from {}")] = N(_remove_the_last_item_from_T);
    natives[S("remove item {} from {}")] = N(_remove_item_T_from_T);
    natives[S("(the) (first) offset of {} in {}")] = N(_the_first_offset_of_T_in_T);
    natives[S("(the) last offset of {} in {}")] = N(_the_last_offset_of_T_in_T);
    natives[S("replace all {} with {} in {}")] = N(_replace_all_T_with_T_in_T);
    natives[S("replace first {} with {} in {}")] = N(_replace_first_T_with_T_in_T);
    natives[S("replace last {} with {} in {}")] = N(_replace_last_T_with_T_in_T);
    natives[S("sort {}")] = N(_sort_T);
}

static void _types(ModuleMap &natives) {
    natives[S("{} as (a/an) int/integer")] = N(_T_as_an_integer);
    natives[S("{} as (a/an) num/number")] = N(_T_as_a_number);
    natives[S("{} as (a/an) str/string")] = N(_T_as_a_string);
    natives[S("{} is (a/an) int/integer")] = N(_T_is_a_integer);
    natives[S("{} is (a/an) num/number")] = N(_T_is_a_number);
    natives[S("{} is (a/an) str/string")] = N(_T_is_a_string);
    natives[S("{} is (a/an) list")] = N(_T_is_a_list);
    natives[S("{} is (a/an) dict/dictionary")] = N(_T_is_a_dictionary);
    natives[S("an empty str/string")] = N(_an_empty_string);
    natives[S("an empty list")] = N(_an_empty_list);
    natives[S("an empty dict/dictionary")] = N(_an_empty_dictionary);
}

static void _dictionary(ModuleMap &natives) {
    natives[S("(the) keys (of) {}")] = N(_the_keys_of_T);
    natives[S("(the) values (of) {}")] = N(_the_values_of_T);
    natives[S("insert item {} with key {} into {}")] = N(_insert_item_T_with_key_T_into_T);
}

static void _list(ModuleMap &natives, std::mt19937_64 &engine,
                  std::function<Integer(Integer)> randomInteger) {
    natives[S("items {} to {} (in/of) {}")] = N(_items_T_to_T_in_T);
    natives[S("(the) first item (in/of) {}")] = N(_the_first_item_in_T);
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

    natives[S("format string {} with {}")] = N(_format_string_T_with_T);

    natives[S("(the) char/character (of) {}")] = N(_character_of_T);
    natives[S("(the) numToChar (of) {}")] = N(_character_of_T);

    natives[S("(the) ord/ordinal (of) {}")] = N(_ordinal_of_T);
    natives[S("(the) charToNum (of) {}")] = N(_ordinal_of_T);
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
    natives[S("(the) asin (of) {}")] = N(_the_func_of_T(asin));
    natives[S("(the) cos (of) {}")] = N(_the_func_of_T(cos));
    natives[S("(the) acos (of) {}")] = N(_the_func_of_T(acos));
    natives[S("(the) tan (of) {}")] = N(_the_func_of_T(tan));
    natives[S("(the) atan (of) {}")] = N(_the_func_of_T(atan));
    natives[S("(the) exp (of) {}")] = N(_the_func_of_T(exp));
    natives[S("(the) exp2 (of) {}")] = N(_the_func_of_T(exp2));
    natives[S("(the) expm1 (of) {}")] = N(_the_func_of_T(expm1));
    natives[S("(the) log2 (of) {}")] = N(_the_func_of_T(log2));
    natives[S("(the) log10 (of) {}")] = N(_the_func_of_T(log10));
    natives[S("(the) log (of) {}")] = N(_the_func_of_T(log));
    natives[S("(the) sqrt (of) {}")] = N(_the_func_of_T(sqrt));
    natives[S("(the) square root (of) {}")] = N(_the_func_of_T(sqrt));
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
