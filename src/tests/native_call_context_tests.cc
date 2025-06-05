//
//  Copyright (c) 2025 James Callender
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

#include <sif/runtime/objects/Native.h>
#include <sif/runtime/VirtualMachine.h>
#include "TestSuite.h"

using namespace sif;

TEST_CASE(NativeCallContext, ErrorMethodWithRanges) {
    VirtualMachine vm;
    SourceLocation callLocation{1, 1, 0};
    Value args[] = {Value(42), Value(std::string("test"))};

    // Create context with argument ranges
    std::vector<SourceRange> ranges = {
        SourceRange{SourceLocation{1, 1, 0}, SourceLocation{10, 1, 0}},  // Call range
        SourceRange{SourceLocation{5, 1, 0}, SourceLocation{7, 1, 0}},   // First arg range
        SourceRange{SourceLocation{8, 1, 0}, SourceLocation{14, 1, 0}}   // Second arg range
    };

    NativeCallContext context(vm, callLocation, args, ranges);

    // Test error() method - should use call location (first range)
    Error error = context.error("test error");
    ASSERT_EQ(error.range.start.position, 1);
    ASSERT_EQ(error.range.start.lineNumber, 1);
    ASSERT_EQ(error.range.end.position, 10);
    ASSERT_EQ(error.range.end.lineNumber, 1);
}

TEST_CASE(NativeCallContext, ErrorMethodWithoutRanges) {
    VirtualMachine vm;
    SourceLocation callLocation{5, 2, 0};
    Value args[] = {Value(42), Value(std::string("test"))};

    // Create context without argument ranges
    NativeCallContext context(vm, callLocation, args, {});

    // Test error() method - should use call location directly
    Error error = context.error("test error");
    ASSERT_EQ(error.range.start.position, 5);
    ASSERT_EQ(error.range.start.lineNumber, 2);
    ASSERT_EQ(error.range.end.position, 5);
    ASSERT_EQ(error.range.end.lineNumber, 2);
}

TEST_CASE(NativeCallContext, ArgumentErrorWithRanges) {
    VirtualMachine vm;
    SourceLocation callLocation{1, 1, 0};
    Value args[] = {Value(42), Value(std::string("test"))};

    std::vector<SourceRange> ranges = {
        SourceRange{SourceLocation{1, 1, 0}, SourceLocation{20, 1, 0}},  // Call range
        SourceRange{SourceLocation{5, 1, 0}, SourceLocation{7, 1, 0}},   // First arg range
        SourceRange{SourceLocation{15, 1, 0}, SourceLocation{19, 1, 0}}  // Second arg range
    };

    NativeCallContext context(vm, callLocation, args, ranges);

    // Test argumentError for first argument
    Error error0 = context.argumentError(0, "expected integer");
    ASSERT_EQ(error0.range.start.position, 5);  // First arg location
    ASSERT_EQ(error0.range.start.lineNumber, 1);
    ASSERT_EQ(error0.range.end.position, 7);
    ASSERT_EQ(error0.range.end.lineNumber, 1);

    // Test argumentError for second argument
    Error error1 = context.argumentError(1, "expected string");
    ASSERT_EQ(error1.range.start.position, 15); // Second arg location
    ASSERT_EQ(error1.range.start.lineNumber, 1);
    ASSERT_EQ(error1.range.end.position, 19);
    ASSERT_EQ(error1.range.end.lineNumber, 1);
}

TEST_CASE(NativeCallContext, ArgumentErrorWithoutRanges) {
    VirtualMachine vm;
    SourceLocation callLocation{10, 3, 0};
    Value args[] = {Value(42), Value(std::string("test"))};

    // Create context without argument ranges
    NativeCallContext context(vm, callLocation, args, {});

    // Test argumentError - should fall back to call location with argument number
    Error error = context.argumentError(0, "expected integer");

    // Should use call location
    ASSERT_EQ(error.range.start.position, 10);
    ASSERT_EQ(error.range.start.lineNumber, 3);

    // Error message should include argument number
    std::string errorMsg = error.value.toString();
    ASSERT_TRUE(errorMsg.find("argument 1") != std::string::npos)
        << "Error message should contain 'argument 1': " << errorMsg;
    ASSERT_TRUE(errorMsg.find("expected integer") != std::string::npos)
        << "Error message should contain the specific error: " << errorMsg;
}

TEST_CASE(NativeCallContext, ArgumentErrorOutOfBounds) {
    VirtualMachine vm;
    SourceLocation callLocation{1, 1, 0};
    Value args[] = {Value(42)};

    std::vector<SourceRange> ranges = {
        SourceRange{SourceLocation{1, 1, 0}, SourceLocation{10, 1, 0}},  // Call range
        SourceRange{SourceLocation{5, 1, 0}, SourceLocation{7, 1, 0}}    // First arg range
    };

    NativeCallContext context(vm, callLocation, args, ranges);

    // Test argumentError with out-of-bounds index
    Error error = context.argumentError(5, "invalid argument");

    // Should fall back to call location with argument number
    ASSERT_EQ(error.range.start.position, 1);
    ASSERT_EQ(error.range.start.lineNumber, 1);

    std::string errorMsg = error.value.toString();
    ASSERT_TRUE(errorMsg.find("argument 6") != std::string::npos)
        << "Error message should contain 'argument 6': " << errorMsg;
}

TEST_CASE(NativeCallContext, FormatStringSupport) {
    VirtualMachine vm;
    SourceLocation callLocation{1, 1, 0};
    Value args[] = {Value(42)};

    NativeCallContext context(vm, callLocation, args, {});

    // Test that format string methods work correctly
    Error error = context.error("Value is {} and type is {}", 42, "integer");

    std::string errorMsg = error.value.toString();
    ASSERT_TRUE(errorMsg.find("Value is 42") != std::string::npos)
        << "Format string should work: " << errorMsg;
    ASSERT_TRUE(errorMsg.find("type is integer") != std::string::npos)
        << "Format string should work: " << errorMsg;
}

TEST_CASE(NativeCallContext, ArgumentErrorFormatString) {
    VirtualMachine vm;
    SourceLocation callLocation{1, 1, 0};
    Value args[] = {Value(42)};

    std::vector<SourceRange> ranges = {
        SourceRange{SourceLocation{1, 1, 0}, SourceLocation{10, 1, 0}},  // Call range
        SourceRange{SourceLocation{5, 1, 0}, SourceLocation{7, 1, 0}}    // First arg range
    };

    NativeCallContext context(vm, callLocation, args, ranges);

    // Test argumentError with format string
    Error error = context.argumentError(0, "expected {}, got {}", "string", "integer");

    ASSERT_EQ(error.range.start.position, 5);  // Should point to argument location

    std::string errorMsg = error.value.toString();
    ASSERT_TRUE(errorMsg.find("expected string") != std::string::npos)
        << "Format string should work: " << errorMsg;
    ASSERT_TRUE(errorMsg.find("got integer") != std::string::npos)
        << "Format string should work: " << errorMsg;
}