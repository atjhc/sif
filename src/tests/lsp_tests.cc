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

#include <sif/lsp/DocumentManager.h>
#include <sif/lsp/SemanticTokens.h>
#include "tests/TestSuite.h"

using namespace sif;
using namespace sif::lsp;

TEST_CASE(LSPTests, DocumentManagerBasics) {
    DocumentManager manager;

    std::string uri = "file:///test.sif";
    std::string content = "set x to 42\nprint x";

    manager.openDocument(uri, content, 1);

    auto doc = manager.getDocument(uri);
    ASSERT_TRUE(doc != nullptr);
    ASSERT_EQ(doc->uri, uri);
    ASSERT_EQ(doc->content, content);
    ASSERT_EQ(doc->version, 1);

    std::string updatedContent = "set y to 100\nprint y";
    manager.updateDocument(uri, updatedContent, 2);

    doc = manager.getDocument(uri);
    ASSERT_TRUE(doc != nullptr);
    ASSERT_EQ(doc->content, updatedContent);
    ASSERT_EQ(doc->version, 2);

    manager.closeDocument(uri);
    doc = manager.getDocument(uri);
    ASSERT_TRUE(doc == nullptr);
}

TEST_CASE(LSPTests, DocumentParsing) {
    DocumentManager manager;

    std::string content = R"(
function greet {name}
    print "Hello, {name}!"
end function

set message to "World"
greet message
)";

    manager.openDocument("file:///test.sif", content, 1);
    auto doc = manager.getDocument("file:///test.sif");

    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->scanner != nullptr);
    ASSERT_TRUE(doc->ast != nullptr);
}

TEST_CASE(LSPTests, SemanticTokensSimple) {
    DocumentManager manager;

    std::string content = "set x to 42";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    ASSERT_TRUE(doc != nullptr);

    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    ASSERT_TRUE(tokens.size() > 0);
    ASSERT_TRUE(tokens.size() % 5 == 0) << "Token data must be multiple of 5";
}

TEST_CASE(LSPTests, SemanticTokensKeywords) {
    DocumentManager manager;

    std::string content = "if x > 5 then\n    print \"yes\"\nend if";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    ASSERT_TRUE(tokens.size() >= 5);

    uint32_t tokenCount = tokens.size() / 5;
    ASSERT_TRUE(tokenCount >= 4) << "Should have at least 4 tokens (if, then, string, end)";
}

TEST_CASE(LSPTests, SemanticTokensStringLiterals) {
    DocumentManager manager;

    std::string content = R"(print "Hello, World!")";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    ASSERT_TRUE(tokens.size() >= 5);

    bool foundString = false;
    for (size_t i = 0; i < tokens.size(); i += 5) {
        uint32_t tokenType = tokens[i + 3];
        if (tokenType == static_cast<uint32_t>(SemanticTokensProvider::TokenType::String)) {
            foundString = true;
            break;
        }
    }

    ASSERT_TRUE(foundString) << "Should find at least one string token";
}

TEST_CASE(LSPTests, SemanticTokensInterpolatedString) {
    DocumentManager manager;

    std::string content = R"(set name to "Alice"
print "Hello, {name}!")";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    ASSERT_TRUE(tokens.size() >= 10);

    int stringTokenCount = 0;
    for (size_t i = 0; i < tokens.size(); i += 5) {
        uint32_t tokenType = tokens[i + 3];
        if (tokenType == static_cast<uint32_t>(SemanticTokensProvider::TokenType::String)) {
            stringTokenCount++;
        }
    }

    ASSERT_TRUE(stringTokenCount >= 2) << "Should find string tokens for interpolated string parts";
}

TEST_CASE(LSPTests, SemanticTokensNumbers) {
    DocumentManager manager;

    std::string content = "set x to 42\nset y to 3.14";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    int numberTokenCount = 0;
    for (size_t i = 0; i < tokens.size(); i += 5) {
        uint32_t tokenType = tokens[i + 3];
        if (tokenType == static_cast<uint32_t>(SemanticTokensProvider::TokenType::Number)) {
            numberTokenCount++;
        }
    }

    ASSERT_EQ(numberTokenCount, 2) << "Should find exactly 2 number tokens";
}

TEST_CASE(LSPTests, SemanticTokensComments) {
    DocumentManager manager;

    std::string content = "-- This is a comment\nset x to 42  -- Another comment";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    int commentTokenCount = 0;
    for (size_t i = 0; i < tokens.size(); i += 5) {
        uint32_t tokenType = tokens[i + 3];
        if (tokenType == static_cast<uint32_t>(SemanticTokensProvider::TokenType::Comment)) {
            commentTokenCount++;
        }
    }

    ASSERT_EQ(commentTokenCount, 2) << "Should find exactly 2 comment tokens";
}

TEST_CASE(LSPTests, SemanticTokensDeltaEncoding) {
    DocumentManager manager;

    std::string content = "set x to 42\nset y to 100";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    ASSERT_TRUE(tokens.size() >= 10);

    uint32_t prevLine = 0;
    uint32_t prevChar = 0;

    for (size_t i = 0; i < tokens.size(); i += 5) {
        uint32_t deltaLine = tokens[i];
        uint32_t deltaChar = tokens[i + 1];
        uint32_t length = tokens[i + 2];

        uint32_t line = prevLine + deltaLine;
        uint32_t character = (deltaLine == 0) ? (prevChar + deltaChar) : deltaChar;

        ASSERT_TRUE(length > 0) << "Token length must be positive";
        ASSERT_TRUE(character < 1000) << "Character position seems unreasonable";

        prevLine = line;
        prevChar = character;
    }
}

TEST_CASE(LSPTests, MultipleDocuments) {
    DocumentManager manager;

    manager.openDocument("file:///test1.sif", "set x to 1", 1);
    manager.openDocument("file:///test2.sif", "set y to 2", 1);
    manager.openDocument("file:///test3.sif", "set z to 3", 1);

    ASSERT_TRUE(manager.getDocument("file:///test1.sif") != nullptr);
    ASSERT_TRUE(manager.getDocument("file:///test2.sif") != nullptr);
    ASSERT_TRUE(manager.getDocument("file:///test3.sif") != nullptr);

    ASSERT_EQ(manager.documents().size(), 3u);

    manager.closeDocument("file:///test2.sif");

    ASSERT_TRUE(manager.getDocument("file:///test1.sif") != nullptr);
    ASSERT_TRUE(manager.getDocument("file:///test2.sif") == nullptr);
    ASSERT_TRUE(manager.getDocument("file:///test3.sif") != nullptr);

    ASSERT_EQ(manager.documents().size(), 2u);
}
