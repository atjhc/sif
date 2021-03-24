#include "parser/Parser.h"

static void parse_tests(TestSuite &suite) {
    for (auto path : suite.files_in("parser")) {
        chatter::ParserConfig config;
        config.fileName = path;

        auto source = suite.file_contents(path);
        suite.assert_true(chatter::Parser().parse(config, source) != nullptr);
    }
}