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
#include "compiler/Module.h"
#include "compiler/Scanner.h"

#include <iostream>
#include <set>
#include <stack>

SIF_NAMESPACE_BEGIN

class SignatureProvider {
  public:
    virtual Result<std::vector<Signature>, Error> signatures(const std::string &name) = 0;
};

struct ParserConfig {
    std::string fileName;
    std::ostream &cerr;

    Strong<Scanner> scanner;
    Strong<Reader> reader;

    Strong<ModuleProvider> moduleProvider;

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

    const std::vector<Signature> &declarations() const;
    const std::vector<Error> &errors() const;

  private:
    using TokenTypes = std::initializer_list<Token::Type>;

    bool isAtEnd();
    bool check(const TokenTypes &types);
    Optional<Token> match(const TokenTypes &types);
    Optional<Token> matchWord();

    Optional<Token> consume(Token::Type type);
    Optional<Token> consumeEnd(Token::Type type);
    Optional<Token> consumeWord();

    bool consumeNewLine();

    Token scan();
    Token advance();
    Token peek();
    Token previous();
    Token synchronize(const TokenTypes &tokenTypes = {Token::Type::NewLine});

    void checkpoint();
    void rewind();
    void commit();

    void beginScope();
    void endScope();

    NoneType emitError(const Error &error);

#if defined(DEBUG)
    void _trace(const std::string &message) const;
    std::string _traceTokens() const;
#endif

    Optional<Signature> parseSignature();

    Owned<Statement> parseBlock(const TokenTypes &endTypes = {});
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

    Result<Owned<Statement>, Error> parseSimpleStatement();
    Result<Owned<Statement>, Error> parseAssignment();
    Result<Owned<Statement>, Error> parseExit();
    Result<Owned<Statement>, Error> parseNext();
    Result<Owned<Statement>, Error> parseReturn();
    Result<Owned<Statement>, Error> parseExpressionStatement();

    Result<Owned<Expression>, Error> parseExpression();
    Result<Owned<Expression>, Error> parseClause();
    Result<Owned<Expression>, Error> parseEquality();
    Result<Owned<Expression>, Error> parseComparison();
    Result<Owned<Expression>, Error> parseList();
    Result<Owned<Expression>, Error> parseRange();
    Result<Owned<Expression>, Error> parseTerm();
    Result<Owned<Expression>, Error> parseFactor();
    Result<Owned<Expression>, Error> parseExponent();
    Result<Owned<Expression>, Error> parseUnary();
    Result<Owned<Expression>, Error> parseCall();
    Result<Owned<Expression>, Error> parseSubscript();
    Result<Owned<Expression>, Error> parsePrimary();
    Result<Owned<Expression>, Error> parseGrouping();
    Result<Owned<Expression>, Error> parseContainerLiteral();

    ParserConfig _config;

    std::vector<Error> _errors;

    struct Scope {
        std::vector<Signature> signatures;
    };
    std::vector<Scope> _scopes;
    std::vector<Signature> _exportedDeclarations;

    struct SignatureDecl {
        Signature signature;
        int depth;
    };
    std::vector<Variable> _variableDecls;

    std::vector<Token> _tokens;
    std::stack<size_t> _saved;
    size_t _index;

    bool _recording;
    bool _parsingRepeat;
    int _parsingDepth;
};

SIF_NAMESPACE_END
