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
#include "parser/Scanner.h"
#include "ast/Expression.h"
#include "ast/Statement.h"
#include "runtime/Error.h"

#include <iostream>
#include <stack>
#include <set>

CH_NAMESPACE_BEGIN

/*

program: block
block: statement*
statement: if | repeat | simpleStatement
simpleStatement: expression NL | set NL | EXIT REPEAT NL | NEXT REPEAT NL
set: SET variable TO expression
repeat: REPEAT { [FOREVER] | WHILE expression | UNTIL expression } NL block NL END [REPEAT]
if: IF expression [NL] THEN { singleThen | NL multiThen }
singleThen: simpleStatement [ [NL] ELSE else ]
multiThen: block { END [IF] NL | else }
else: ELSE { statement | NL block END [IF] NL }

expression: clause
clause: equality ((AND | OR) equality)*
equality: comparison (("=" | "!=" | "is" | "is not") comparison)*
comparison: term (("<" | ">" | "<=" | ">=") term)*
term: factor (("+" | "-") factor)*
factor: exponent (("*" | "/" | "%") exponent)*
exponent: unary ("^" unary)*
unary: primary | ("!" | NOT) unary
primary: NUMBER | STRING | TRUE | FALSE | EMPTY | "(" expression ")"

*/

struct ParserConfig {
    std::string fileName;
    std::ostream &cerr;

#if defined(DEBUG)
    bool enableTracing = false;
#endif

    bool disableNatives = false;

    ParserConfig(const std::string &n = "<stdin>", std::ostream &cerr = std::cerr)
        : fileName(n), cerr(cerr) {}
};

class Parser {
  public:
    Parser(const ParserConfig &config, Scanner &scanner);

    Owned<Statement> parse();
    FunctionSignature parseFunctionSignature();

    void add(const FunctionSignature &signature);

    const std::vector<SyntaxError> &errors();

  private:
    bool _isAtEnd();
    bool _check(const std::initializer_list<Token::Type> &types);
    Optional<Token> _match(const std::initializer_list<Token::Type> &types);
    Token _consume(Token::Type type, const std::string &error);
    Token _consumeEnd(Token::Type type);
    Token _consumeNewLine();
    Token& _scan();
    Token& _advance();
    Token& _peek();
    Token& _previous();
    void _synchronize();

    void _checkpoint();
    void _rewind();
    void _commit();

    bool _matchSignature(const FunctionSignature &signature, std::vector<Optional<Token>> &tokens, std::vector<Owned<Expression>> &arguments);
    bool _matchTerm(const FunctionSignature::Term &term, std::vector<Optional<Token>> &tokens, std::vector<Owned<Expression>> &arguments);

#if defined(DEBUG)
    void _trace(const std::string &message);
#endif

    FunctionSignature _parseFunctionSignature();

    Owned<Statement> _parseBlock(const std::initializer_list<Token::Type> &endTypes = {});
    Owned<Statement> _parseStatement();
    Owned<Statement> _parseSimpleStatement();
    Owned<Statement> _parseFunction();
    Owned<Statement> _parseIf();
    Owned<Statement> _parseRepeat();
    Owned<Statement> _parseAssignment();
    Owned<Statement> _parseExit();
    Owned<Statement> _parseNext();
    Owned<Statement> _parseReturn();
    Owned<Statement> _parseExpressionStatement();

    Owned<Expression> _parseExpression();
    Owned<Expression> _parseClause();
    Owned<Expression> _parseEquality();
    Owned<Expression> _parseComparison();
    Owned<Expression> _parseList();
    Owned<Expression> _parseTerm();
    Owned<Expression> _parseFactor();
    Owned<Expression> _parseExponent();
    Owned<Expression> _parseUnary();
    Owned<Expression> _parsePrimary();
    Owned<Expression> _parseCall();

    ParserConfig _config;
    Scanner &_scanner;
    std::vector<SyntaxError> _errors;

    std::set<FunctionSignature> _functionDecls;
    std::vector<Variable> _variableDecls;

    std::vector<Token> _tokens;
    std::stack<size_t> _saved;
    size_t _index;

    bool _recording;
    bool _parsingRepeat;
};

CH_NAMESPACE_END
