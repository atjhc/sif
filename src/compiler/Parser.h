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

    Strong<Scanner> scanner;
    Strong<Reader> reader;

#if defined(DEBUG)
    bool enableTracing = false;
#endif

    ParserConfig(const std::string &n = "<stdin>", std::ostream &cerr = std::cerr)
        : fileName(n), cerr(cerr) {}
};

class Parser {
  public:
    Parser(const ParserConfig &config);
    ~Parser();

    Owned<Statement> statement();
    Optional<Signature> signature();

    void declare(const Signature &signature);

    const std::vector<Signature> &declarations();
    const std::vector<ParseError> &errors();

  private:
    using TokenList = std::initializer_list<Token::Type>;

    bool isAtEnd();
    bool check(const std::initializer_list<Token::Type> &types);
    Optional<Token> match(const std::initializer_list<Token::Type> &types);
    Optional<Token> matchWord();

    Optional<Token> consume(Token::Type type);
    Optional<Token> consumeEnd(Token::Type type);
    Optional<Token> consumeWord();

    bool consumeNewLine();

    Token scan();
    Token advance();
    Token peek();
    Token previous();
    Token synchronize(const TokenList &tokenTypes = {Token::Type::NewLine});

    void checkpoint();
    void rewind();
    void commit();

    void beginScope();
    void endScope();

    NoneType emitError(const ParseError &error);

#if defined(DEBUG)
    void _trace(const std::string &message) const;
    std::string _traceTokens() const;
#endif

    Optional<Signature> parseSignature();

    Owned<Statement> parseBlock(const TokenList &endTypes = {});
    Owned<Statement> parseStatement();
    Owned<Statement> parseFunction();
    Owned<Statement> parseIf();
    Owned<Statement> parseTry();
    Owned<Statement> parseUse();
    Owned<Statement> parseUsing();
    Owned<Statement> parseRepeat();
    Owned<Statement> parseRepeatForever();
    Owned<Statement> parseRepeatConditional();
    Owned<Statement> parseRepeatFor();

    Result<Owned<Statement>, ParseError> parseSimpleStatement();
    Result<Owned<Statement>, ParseError> parseAssignment();
    Result<Owned<Statement>, ParseError> parseExit();
    Result<Owned<Statement>, ParseError> parseNext();
    Result<Owned<Statement>, ParseError> parseReturn();
    Result<Owned<Statement>, ParseError> parseExpressionStatement();

    Result<Owned<Expression>, ParseError> parseExpression();
    Result<Owned<Expression>, ParseError> parseClause();
    Result<Owned<Expression>, ParseError> parseEquality();
    Result<Owned<Expression>, ParseError> parseComparison();
    Result<Owned<Expression>, ParseError> parseList();
    Result<Owned<Expression>, ParseError> parseRange();
    Result<Owned<Expression>, ParseError> parseTerm();
    Result<Owned<Expression>, ParseError> parseFactor();
    Result<Owned<Expression>, ParseError> parseExponent();
    Result<Owned<Expression>, ParseError> parseUnary();
    Result<Owned<Expression>, ParseError> parseCall();
    Result<Owned<Expression>, ParseError> parseSubscript();
    Result<Owned<Expression>, ParseError> parsePrimary();
    Result<Owned<Expression>, ParseError> parseGrouping();
    Result<Owned<Expression>, ParseError> parseContainerLiteral();

    ParserConfig _config;

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
