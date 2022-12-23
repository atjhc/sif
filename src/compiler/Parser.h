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

struct ParserConfig {
    Scanner &scanner;
    Reader &reader;
    ModuleProvider &moduleProvider;

#if defined(DEBUG)
    bool enableTracing = false;
#endif
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

    struct Scope {
        std::vector<Signature> signatures;
        Set<std::string> variables;
    };

    struct Grammar {
        Owned<Grammar> argument;
        Mapping<std::string, Owned<Grammar>> terms;
        Optional<Signature> signature;

        bool insert(const Signature &signature, std::vector<Signature::Term>::const_iterator term);
        bool insert(const Signature &signature) { return insert(signature, signature.terms.cbegin()); }

        bool isLeaf() const;
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
    Result<Owned<Expression>, Error> parseVariable();
    Result<Owned<Expression>, Error> parseGrouping();
    Result<Owned<Expression>, Error> parseContainerLiteral();

    ParserConfig _config;

    std::vector<Error> _errors;

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
};

SIF_NAMESPACE_END
