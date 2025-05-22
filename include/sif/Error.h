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

#include <sif/ast/Node.h>
#include <sif/compiler/Token.h>
#include <sif/runtime/Value.h>

#include <format>

SIF_NAMESPACE_BEGIN

namespace Errors {
inline constexpr std::string_view DuplicateArgumentNames =
    "duplicate argument names in function declaration";
inline constexpr std::string_view CantOpenFile = "can't open file {}";
inline constexpr std::string_view EmptyInterpolation = "empty interpolation";
inline constexpr std::string_view ExpectedANewLine = "expected a new line";
inline constexpr std::string_view ExpectedAnExpression = "expected an expression";
inline constexpr std::string_view ExpectedATypeName = "expected a type name";
inline constexpr std::string_view ExpectedAVariableName = "expected a variable name";
inline constexpr std::string_view ExpectedColon = "expected “:”";
inline constexpr std::string_view ExpectedColonCommaOrBracket = "expected “:”, “,”, or “]”";
inline constexpr std::string_view ExpectedEnd = "expected “end”";
inline constexpr std::string_view ExpectedEndOrElse = "expected “end” or “else”";
inline constexpr std::string_view ExpectedForeverWhileUntilFor =
    "expected “forever”, “while”, “until”, “for”, or a new line";
inline constexpr std::string_view ExpectedIn = "expected “in”";
inline constexpr std::string_view ExpectedNewLineOrEndOfScript =
    "expected a new line or end of script";
inline constexpr std::string_view ExpectedRepeat = "expected “repeat”";
inline constexpr std::string_view ExpectedRightBracket = "expected “]”";
inline constexpr std::string_view ExpectedRightCurlyBrace = "expected “}”";
inline constexpr std::string_view ExpectedRightParens = "expected “)”";
inline constexpr std::string_view ExpectedStringOrWord = "expected a string or word";
inline constexpr std::string_view ExpectedThen = "expected “then”";
inline constexpr std::string_view ExpectedTo = "expected “to”";
inline constexpr std::string_view ExpectedWord = "expected a word";
inline constexpr std::string_view ExpectedWordParenOrCurly = "expected a word, “(”, or “{”";
inline constexpr std::string_view UnderscoreNotAllowed = "“_” may not be used as a variable name";
inline constexpr std::string_view UnexpectedExit = "unexpected “exit” outside repeat block";
inline constexpr std::string_view UnexpectedNext = "unexpected “next” outside repeat block";
inline constexpr std::string_view UnexpectedToken = "unexpected {}";
inline constexpr std::string_view UnknownExpression = "unknown expression “{}”";
inline constexpr std::string_view UnknownCharacter = "unknown character “{}”";
inline constexpr std::string_view UnterminatedInterpolation = "unterminated interpolation";
inline constexpr std::string_view UnterminatedString = "unterminated string";
inline constexpr std::string_view TooManyAssignmentTargets = "too many assignment targets";
inline constexpr std::string_view TooManyLocalVariables = "too many local variables";
inline constexpr std::string_view UnusedLocalVariable =
    "unused local variable “{}” will always be empty";
inline constexpr std::string_view ValueOutOfRange = "value is either too large or too small";
inline constexpr std::string_view CircularModuleImport = "circular module import";
inline constexpr std::string_view ModuleNotFound = "module “{}” not found";
inline constexpr std::string_view ExpectedListStringDictRange =
    "expected a list, string, dictionary, or range";
inline constexpr std::string_view BoundsMismatch =
    "lower bound must be less than or equal to the upper bound";
inline constexpr std::string_view ExpectedInteger = "expected an integer";
inline constexpr std::string_view UnexpectedTypeForCall = "unexpected type for function call";
inline constexpr std::string_view ProgramHalted = "program halted";
inline constexpr std::string_view MismatchedTypes = "mismatched types: {} {} {}";
inline constexpr std::string_view ExpectedTrueOrFalse = "expected true or false";
inline constexpr std::string_view ExpectedEnumerator = "expected an enumerator";
inline constexpr std::string_view ExpectedList = "expected a list but got {}";
inline constexpr std::string_view ExpectedNumber = "expected a number but got {}";
inline constexpr std::string_view UnpackListMismatch = "expected {} values but got {}";
inline constexpr std::string_view DivideByZero = "divide by zero";
inline constexpr std::string_view InvalidFunctionSignature = "invalid function signature";
} // namespace Errors

struct Error {
    Error(const Value &value) : value(value) {}
    Error(const char *str) : Error(std::string(str)) {}

    template <class... Args>
    Error(std::format_string<Args...> fmt, Args &&...args)
        : Error(std::format(fmt, std::forward<Args>(args)...)) {}

    Error(const SourceRange &range, const Value &value) : range(range), value(value) {}
    Error(const SourceLocation &location, const Value &value)
        : range{location, location}, value(value) {}

    Error(const SourceLocation &location, const char *str)
        : Error(location, Value(std::string(str))) {}
    Error(const SourceRange &range, const char *str) : Error(range, Value(std::string(str))) {}

    template <class... Args>
    Error(const SourceLocation &location, std::format_string<Args...> fmt, Args &&...args)
        : Error(location, std::format(fmt, std::forward<Args>(args)...)) {}

    template <class... Args>
    Error(const SourceRange &range, std::format_string<Args...> fmt, Args &&...args)
        : Error(range, std::format(fmt, std::forward<Args>(args)...)) {}

    std::string what() const { return value.toString(); }

    SourceRange range;
    Value value;
};

SIF_NAMESPACE_END
