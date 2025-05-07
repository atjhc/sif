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

#include <sif/Common.h>
#include <sif/Error.h>
#include <sif/ast/Expression.h>
#include <sif/ast/Statement.h>
#include <sif/compiler/Grammar.h>
#include <sif/compiler/Module.h>
#include <sif/compiler/Reporter.h>
#include <sif/compiler/Scanner.h>

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

    /// @brief Parse and return a Statement object.
    /// @return Returns a strong reference to the Statement object.
    Strong<Statement> statement();

    /// @brief Parse and return a signature object.
    /// @return Returns an optional Signature object which will be set if the parser was successful.
    Optional<Signature> signature();

    /// @return Returns a bool value indicating if the parser failed to parse.
    bool failed() const;

    /// @brief Declare the signature as a valid function call.
    /// @param signature Signature to be matched for function calls.
    void declare(const Signature &signature);

    /// @brief Declare a list of signatures as valid function calls.
    /// @param signatures The list of Signature objects to be matched for function calls.
    void declare(const std::vector<Signature> &signatures);

    /// @brief Declare a variable
    /// @param variable The variable name
    void declare(const std::string &variable);

    /// @brief Declare a set of variables.
    /// @param variables The Set of variables
    void declare(const Set<std::string> &variables);

    /// @return Returns the list of internally declared functions after parsing.
    const std::vector<Signature> &declarations() const;

    /// @return Returns the set of declared variables after parsing.
    const Set<std::string> &variables() const;

    /// @return Returns the list of all internally and externally declared functions after parsing.
    const std::vector<Signature> &signatures() const;

    /// @return Returns the list of ranges where comments were detected.
    const std::vector<SourceRange> &commentRanges() const;

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
    Token synchronizeTo(const TokenTypes &tokenTypes = {Token::Type::NewLine});

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

    Signature parseSignature();

    Strong<Statement> parseBlock(const TokenTypes &endTypes = {});
    Strong<Statement> parseStatement();
    Strong<Statement> parseFunction();
    Strong<Statement> parseIf();
    Strong<Statement> parseTry();
    Strong<Statement> parseUse();
    Strong<Statement> parseUsing();
    Strong<Statement> parseRepeat();
    Strong<Repeat> parseRepeatForever();
    Strong<RepeatCondition> parseRepeatCondition();
    Strong<RepeatFor> parseRepeatFor();

    Result<Strong<Statement>, Error> parseSimpleStatement();
    Result<Strong<Statement>, Error> parseAssignment();
    Result<Strong<Statement>, Error> parseExit();
    Result<Strong<Statement>, Error> parseNext();
    Result<Strong<Statement>, Error> parseReturn();
    Result<Strong<Statement>, Error> parseExpressionStatement();

    Strong<Expression> parseExpression();
    Strong<Expression> parseClause();
    Strong<Expression> parseEquality();
    Strong<Expression> parseComparison();
    Strong<Expression> parseList();
    Strong<Expression> parseRange();
    Strong<Expression> parseTerm();
    Strong<Expression> parseFactor();
    Strong<Expression> parseExponent();
    Strong<Expression> parseUnary();
    Strong<Expression> parseCallPostfix();
    Strong<Expression> parseCallPrefix();
    Strong<Expression> parseCall(bool);
    Strong<Expression> parseSubscript();
    Strong<Expression> parsePrimary();
    Strong<Expression> parseInterpolation();
    Strong<Expression> parseVariable();
    Strong<Expression> parseGrouping();
    Strong<Expression> parseContainerLiteral();

    ParserConfig _config;

    std::vector<Scope> _scopes;
    std::vector<Signature> _exportedDeclarations;
    std::vector<SourceRange> _commentRanges;

    sif::Strong<Grammar> _grammar;
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
