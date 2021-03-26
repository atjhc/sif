#include "utilities/strings.h"

static void string_tests(TestSuite &s) {
    auto escape = chatter::string_from_escaped_string;

    s.assert_eq(escape("Hello, World!"), "Hello, World!");
    s.assert_eq(escape("Hello\\n World!"), "Hello\n World!");
    s.assert_eq(escape("Hello\\\', World!"), "Hello\', World!");
    s.assert_eq(escape("Hello\\\", World!"), "Hello\", World!");
    s.assert_eq(escape("Hello\\\\, World!"), "Hello\\, World!");
    s.assert_eq(escape("Hello\\f, World!"), "Hello\f, World!");
    s.assert_eq(escape("Hello\\r, World!"), "Hello\r, World!");
    s.assert_eq(escape("Hello\\t, World!"), "Hello\t, World!");
    s.assert_eq(escape("Hello\\v, World!"), "Hello\v, World!");
    s.assert_eq(escape("Hello, World!\\040"), "Hello, World!\040");
    s.assert_eq(escape("Hello, World!\\40"), "Hello, World!\40");
    s.assert_eq(escape("Hello\\x32, World!"), "Hello\x32, World!");
}
