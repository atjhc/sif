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

#include <filesystem>

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

TEST_CASE(LSPTests, CompletionVariables) {
    DocumentManager manager;

    std::string content = R"(
set firstName to "Alice"
set lastName to "Smith"
set age to 30
)";

    manager.openDocument("file:///test.sif", content, 1);
    auto doc = manager.getDocument("file:///test.sif");

    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->variables.count("firstname") > 0);
    ASSERT_TRUE(doc->variables.count("lastname") > 0);
    ASSERT_TRUE(doc->variables.count("age") > 0);
    ASSERT_EQ(doc->variables.size(), 3u);
}

TEST_CASE(LSPTests, CompletionFunctions) {
    DocumentManager manager;

    std::string content = R"(
function greet {name}
    print "Hello, {name}!"
end function

function the square of {n}
    return n * n
end function
)";

    manager.openDocument("file:///test.sif", content, 1);
    auto doc = manager.getDocument("file:///test.sif");

    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->signatures.size() > 0);

    bool foundGreet = false;
    bool foundSquare = false;

    for (const auto &sig : doc->signatures) {
        std::string desc = sig.description();
        if (desc.find("greet") != std::string::npos &&
            desc.find("{name:}") != std::string::npos) {
            foundGreet = true;
        }
        if (desc.find("square") != std::string::npos &&
            desc.find("{n:}") != std::string::npos) {
            foundSquare = true;
        }
    }

    ASSERT_TRUE(foundGreet) << "Should find 'greet' function signature";
    ASSERT_TRUE(foundSquare) << "Should find 'square' function signature";
}

TEST_CASE(LSPTests, CompletionBuiltinFunctions) {
    DocumentManager manager;

    std::string content = "set x to 42";
    manager.openDocument("file:///test.sif", content, 1);
    auto doc = manager.getDocument("file:///test.sif");

    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->signatures.size() > 0) << "Should have built-in function signatures";

    bool foundPrint = false;
    bool foundTypeOf = false;

    for (const auto &sig : doc->signatures) {
        std::string desc = sig.description();
        if (desc.find("print") != std::string::npos) foundPrint = true;
        if (desc.find("type name") != std::string::npos) foundTypeOf = true;
    }

    ASSERT_TRUE(foundPrint) << "Should find 'print' built-in function";
    ASSERT_TRUE(foundTypeOf) << "Should find 'type name' built-in function";
}

TEST_CASE(LSPTests, CompletionSnippets) {
    DocumentManager manager;

    std::string content = R"(
function greet {name}
    print "Hello, {name}!"
end function

function the square of {n}
    return n * n
end function
)";

    manager.openDocument("file:///test.sif", content, 1);
    auto doc = manager.getDocument("file:///test.sif");

    ASSERT_TRUE(doc != nullptr);
    ASSERT_TRUE(doc->signatures.size() > 0);

    // Verify that user-defined signatures have argument placeholders
    bool foundGreet = false;
    bool foundSquare = false;

    for (const auto &sig : doc->signatures) {
        std::string desc = sig.description();
        if (desc.find("greet") != std::string::npos &&
            desc.find("{name:}") != std::string::npos) {
            foundGreet = true;
        }
        if (desc.find("square") != std::string::npos &&
            desc.find("{n:}") != std::string::npos) {
            foundSquare = true;
        }
    }

    ASSERT_TRUE(foundGreet) << "Should find greet with {name:} parameter";
    ASSERT_TRUE(foundSquare) << "Should find square with {n:} parameter";
}

TEST_CASE(LSPTests, CompletionVariations) {
    DocumentManager manager;

    // Create a document that will load builtin signatures with optionals and choices
    std::string content = "set x to 42";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    ASSERT_TRUE(doc != nullptr);

    // Find a signature with an optional term, like "(the) sin of {angle}"
    bool foundSinWithThe = false;

    for (const auto &sig : doc->signatures) {
        std::string desc = sig.description();
        if (desc.find("sin") != std::string::npos &&
            desc.find("{angle") != std::string::npos) {
            // This signature has an optional "the"
            // We should find it generates both variations
            foundSinWithThe = true; // Signature exists
        }
    }

    ASSERT_TRUE(foundSinWithThe) << "Should find sin signature with optional 'the'";

    // The actual variation testing will happen in completion handling
    // For now, just verify the signatures exist
}

TEST_CASE(LSPTests, ModuleImports) {
    DocumentManager manager;

    // Set workspace root to the transcripts/modules directory
    std::filesystem::path modulesPath = std::filesystem::path(suite.config.resourcesPath) / "transcripts" / "modules";
    std::string workspaceRoot = "file://" + std::filesystem::absolute(modulesPath).string();
    manager.setWorkspaceRoot(workspaceRoot);

    // Open a document that imports module1.sif
    std::string content = R"(use "module1.sif"
set x to 42)";

    std::string docUri = "file://" + (std::filesystem::absolute(modulesPath) / "test_use.sif").string();
    manager.openDocument(docUri, content, 1);
    auto doc = manager.getDocument(docUri);

    ASSERT_TRUE(doc != nullptr);

    // Check that imported function "say hello" is in signatures
    bool foundSayHello = false;
    for (const auto &sig : doc->signatures) {
        std::string desc = sig.description();
        if (desc.find("say hello") != std::string::npos) {
            foundSayHello = true;
            break;
        }
    }

    ASSERT_TRUE(foundSayHello) << "Should find 'say hello' from imported module1.sif";
}

TEST_CASE(LSPTests, UnicodeSemanticTokens) {
    DocumentManager manager;

    // Test with multi-byte UTF-8 characters
    // Chinese character 母 (mother) is 3 bytes in UTF-8
    // Line: "set 母 to 42"
    //       012345678901  (character positions, not bytes)
    std::string content = "set 母 to 42";
    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    ASSERT_TRUE(doc != nullptr);

    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    // Tokens should be encoded as:
    // [deltaLine, deltaChar, length, tokenType, tokenModifiers]
    ASSERT_TRUE(tokens.size() >= 15) << "Should have at least 3 tokens (set, 母, 42)";

    // In "set 母 to 42", the variable 母 is at character position 4 (after "set ")
    // Token 0: "set" at position 0
    // Token 1: "母" at position 4 (delta from position 0)
    size_t varTokenIndex = 5; // Second token in the array (first is "set")
    uint32_t deltaChar = tokens[varTokenIndex + 1];
    uint32_t length = tokens[varTokenIndex + 2];

    ASSERT_EQ(deltaChar, 4u) << "Variable should be at character position 4 (after 'set ')";
    ASSERT_EQ(length, 1u) << "Variable 母 should have length 1 (character count, not bytes)";
}

TEST_CASE(LSPTests, UnicodeMultilineSemanticTokens) {
    DocumentManager manager;

    // Test with unicode on multiple lines
    // Each line has unicode characters
    std::string content = R"(set 母 to "mother"
set мати to "mother"
set mère to "mother")";

    manager.openDocument("file:///test.sif", content, 1);

    auto doc = manager.getDocument("file:///test.sif");
    ASSERT_TRUE(doc != nullptr);

    auto tokens = SemanticTokensProvider::encodeTokens(*doc);

    // Should have tokens for all three lines
    ASSERT_TRUE(tokens.size() > 0);
    ASSERT_TRUE(tokens.size() % 5 == 0) << "Token data must be multiple of 5";

    // Verify we can decode without crashes (positions are reasonable)
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
