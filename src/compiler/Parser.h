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
#include "compiler/Grammar.h"
#include "compiler/Module.h"
#include "compiler/Reporter.h"
#include "compiler/Scanner.h"

#include <iostream>
#include <set>
#include <stack>

SIF_NAMESPACE_BEGIN

struct ParserConfig {
    Scanner &scanner;
    Reader &reader;
    ModuleProvider &moduleProvider;
    Reporter &reporter;

#if defined(DEBUG)
    bool enableTracing = false;
#endif
};

class Parser {
  public:
    Parser(const ParserConfig &config);
    ~Parser();

    Strong<Statement> statement();
    Optional<Signature> signature();

    void declare(const Signature &signature);
    void declare(const std::string &variable);

    const std::vector<Signature> &declarations() const;

  private:
    using TokenTypes = std::initializer_list<Token::Type>;

    struct Scope {
        std::vector<Signature> signatures;
        Set<std::string> variables;
    };

    bool isAtEnd();
    bool check(const TokenTypes &types);
    Optional<Token> match(const TokenTypes &types);

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

    void beginScope(const Scope &scope = Scope());
    void endScope();

    NoneType emitError(const Error &error);

#if defined(DEBUG)
    void _trace(const std::string &message) const;
    std::string _traceTokens() const;
#endif

    Optional<Signature> parseSignature();

    Strong<Statement> parseBlock(const TokenTypes &endTypes = {});
    Strong<Statement> parseStatement();
    Strong<Statement> parseFunction();
    Strong<Statement> parseIf();
    Strong<Statement> parseTry();
    Strong<Statement> parseUse();
    Strong<Statement> parseUsing();
    Strong<Statement> parseRepeat();
    Strong<Statement> parseRepeatForever();
    Strong<Statement> parseRepeatConditional();
    Strong<Statement> parseRepeatFor();

    Result<Strong<Statement>, Error> parseSimpleStatement();
    Result<Strong<Statement>, Error> parseAssignment();
    Result<Strong<Statement>, Error> parseExit();
    Result<Strong<Statement>, Error> parseNext();
    Result<Strong<Statement>, Error> parseReturn();
    Result<Strong<Statement>, Error> parseExpressionStatement();

    Result<Strong<Expression>, Error> parseExpression();
    Result<Strong<Expression>, Error> parseClause();
    Result<Strong<Expression>, Error> parseEquality();
    Result<Strong<Expression>, Error> parseComparison();
    Result<Strong<Expression>, Error> parseList();
    Result<Strong<Expression>, Error> parseRange();
    Result<Strong<Expression>, Error> parseTerm();
    Result<Strong<Expression>, Error> parseFactor();
    Result<Strong<Expression>, Error> parseExponent();
    Result<Strong<Expression>, Error> parseUnary();
    Result<Strong<Expression>, Error> parseCall();
    Result<Strong<Expression>, Error> parseSubscript();
    Result<Strong<Expression>, Error> parsePrimary();
    Result<Strong<Expression>, Error> parseVariable();
    Result<Strong<Expression>, Error> parseGrouping();
    Result<Strong<Expression>, Error> parseContainerLiteral();

    ParserConfig _config;

    std::vector<Scope> _scopes;
    std::vector<Signature> _exportedDeclarations;

    Grammar _grammar;
    Set<std::string> _variables;

    std::vector<Token> _tokens;
    std::stack<size_t> _saved;
    size_t _index;

    bool _recording;
    bool _parsingRepeat;
    int _parsingDepth;

    bool _failed;
};

SIF_NAMESPACE_END
