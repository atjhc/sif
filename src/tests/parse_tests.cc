#include "parser/Parser.h"

static void parse_tests(TestSuite &suite) {

    for (auto path : suite.files_in("parser")) {
        chatter::ParserConfig config(path);
        chatter::Parser parser(config);

        auto source = suite.file_contents(path);
        suite.assert_true(parser.parseScript(source) != nullptr);
    }
}
