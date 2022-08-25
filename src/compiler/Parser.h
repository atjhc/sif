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

#include "Common.h"
#include "Error.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "compiler/Scanner.h"

#include <iostream>
#include <set>
#include <stack>

SIF_NAMESPACE_BEGIN

struct ParserConfig {
    std::string fileName;
    std::ostream &cerr;

#if defined(DEBUG)
    bool enableTracing = false;
#endif

    ParserConfig(const std::string &n = "<stdin>", std::ostream &cerr = std::cerr)
        : fileName(n), cerr(cerr) {}
};

class Parser {
  public:
    Parser(const ParserConfig &config, Strong<Scanner> scanner, Strong<Reader> reader);
    ~Parser();

    Owned<Statement> statement();
    Optional<Signature> signature();

    void declare(const Signature &signature);

    const std::vector<Signature> &declarations();
    const std::vector<ParseError> &errors();

  private:
    bool isAtEnd();
    bool check(const std::initializer_list<Token::Type> &types);
    Optional<Token> match(const std::initializer_list<Token::Type> &types);
    Optional<Token> matchWord();
    Token consume(Token::Type type, const std::string &error);
    Token consumeEnd(Token::Type type);

    bool consumeNewLine();

    Token consumeWord(const std::string &message = "expected a word");
    Token scan();
    Token advance();
    Token peek();
    Token previous();
    void synchronize();

    void checkpoint();
    void rewind();
    void commit();

    void beginScope();
    void endScope();

#if defined(DEBUG)
    void _trace(const std::string &message);
#endif

    Optional<Signature> parseSignature();

    Owned<Statement> parseBlock(const std::initializer_list<Token::Type> &endTypes = {});
    Owned<Statement> parseStatement();
    Owned<Statement> parseSimpleStatement();
    Owned<Statement> parseFunction();
    Owned<Statement> parseIf();
    Owned<Statement> parseRepeat();
    Owned<Statement> parseAssignment();
    Owned<Statement> parseExit();
    Owned<Statement> parseNext();
    Owned<Statement> parseReturn();
    Owned<Statement> parseExpressionStatement();

    Owned<Expression> parseExpression();
    Owned<Expression> parseClause();
    Owned<Expression> parseEquality();
    Owned<Expression> parseComparison();
    Owned<Expression> parseList();
    Owned<Expression> parseRange();
    Owned<Expression> parseTerm();
    Owned<Expression> parseFactor();
    Owned<Expression> parseExponent();
    Owned<Expression> parseUnary();
    Owned<Expression> parseCall();
    Owned<Expression> parseSubscript();
    Owned<Expression> parsePrimary();
    Owned<Expression> parseGrouping();
    Owned<Expression> parseListLiteral();
    Owned<Expression> parseDictionaryLiteral();

    ParserConfig _config;
    Strong<Scanner> _scanner;
    Strong<Reader> _reader;

    std::vector<ParseError> _errors;

    struct SignatureDecl {
        Signature signature;
        int depth;
    };
    std::vector<SignatureDecl> _signatureDecls;
    std::vector<Variable> _variableDecls;
    int _depth;

    std::vector<Token> _tokens;
    std::stack<size_t> _saved;
    size_t _index;

    bool _recording;
    bool _parsingRepeat;
    int _parsingDepth;
};

SIF_NAMESPACE_END
