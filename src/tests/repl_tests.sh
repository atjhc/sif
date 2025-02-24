#!/bin/bash

SIF_BINARY="$1 -i"
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local description="$1"
    local command="$2"
    local expected_output="$3"
    local expected_exit_code=0

    echo -n "$description ... "

    output=$(eval "$command" 2>&1)
    exit_code=$?

    if [[ $exit_code -ne $expected_exit_code ]]; then
        echo "FAILED (exit code $exit_code, expected $expected_exit_code)"
        echo "Command: $command"
        echo "Output: $output"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    if [[ -n "$expected_output" && "$output" != *"$expected_output"* ]]; then
        echo "FAILED (output mismatch)"
        echo "Expected: $expected_output"
        echo "Got: $output"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return
    fi

    echo "PASSED"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

echo "Running Sif REPL tests..."

# Test: Empty input should not crash
run_test "Empty input" "echo -n '' | $SIF_BINARY" ""

# Test: A simple expression
run_test "Simple expression" "echo 'print \"hello\"' | $SIF_BINARY" "hello"

# Test: Global variables persist
run_test "Globals" "echo -e 'set a to 10\nprint a' | $SIF_BINARY" "10"

# Test: Exiting the REPL
run_test "Quit command" "echo 'quit' | $SIF_BINARY" ""

echo ""
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"

if [[ $TESTS_FAILED -ne 0 ]]; then
    exit 1
else
    exit 0
fi