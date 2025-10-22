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

#include "tests/TestSuite.h"
#include "utilities/strings.h"
#include <sif/compiler/Parser.h>
#include <sif/compiler/Scanner.h>
#include <sif/runtime/ModuleLoader.h>
#include "utilities/devnull.h"
#include <sif/ast/SourceAnnotator.h>
#include "utilities/chunk.h"

using namespace sif;

static std::string stringForAnnotationKind(Annotation::Kind kind) {
switch (kind) {
    case Annotation::Kind::Control: return "control";
    case Annotation::Kind::Comment: return "comment";
    case Annotation::Kind::StringLiteral: return "string";
    case Annotation::Kind::NumberLiteral: return "number";
    case Annotation::Kind::Call: return "call";
    case Annotation::Kind::Operator: return "operator";
    case Annotation::Kind::Variable: return "variable";
    case Annotation::Kind::Module: return "module";
    }
    // Unreachable, but GCC requires a return statement
    return "unknown";
}

static std::string gather(const std::string &source, const std::string &context) {
    const std::string search = "(-- " + context + "\n";
    std::ostringstream ss;
    size_t offset = 0;
    while (true) {
        auto start = source.find(search, offset);
        if (start == source.npos) {
            break;
        }
        start += search.size();
        offset = start;
        auto end = source.find("--)", offset);
        offset = end;
        ss << source.substr(start, end - start);
    }
    return ss.str();
}

TEST_CASE(Annotations, All) {
    for (auto pstr : suite.all_files_in("annotations")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".sif") {
            continue;
        }

        auto sourceOpt = suite.file_contents(path);
        ASSERT_TRUE(sourceOpt.has_value());
        if (!sourceOpt) {
            continue;
        }
        auto source = sourceOpt.value();
        auto expectedAnnotations = gather(source, "annotations");

        auto scanner = Scanner();
        auto reader = StringReader(source);
        auto loader = ModuleLoader();
        auto reporter = IOReporter(devnull);

        ParserConfig config{scanner, reader, loader, reporter};
        Parser parser(config);

        auto statement = parser.statement();
        SourceAnnotator annotator;
        auto annotations = annotator.annotate(*statement);

        std::ostringstream ss;
        for (auto &&annotation : annotations) {
            ss  << stringForAnnotationKind(annotation.kind) << " "
                << range_chunk(chunk::type::character, annotation.range.start.offset, annotation.range.end.offset - 1, source).get()
                << std::endl;
        }

        ASSERT_EQ(ss.str(), expectedAnnotations) << path << " failed the output check:" << std::endl
        << "Expected: " << std::endl
        << expectedAnnotations << std::endl
        << "Got: " << std::endl
        << ss.str() << std::endl;
    }
}
