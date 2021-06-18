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

    ParserConfig(const std::string &n = "<stdin>", std::ostream &cerr = std::cerr)
        : fileName(n), cerr(cerr) {}
};

class Parser {
  public:
    Parser(const ParserConfig &config, Scanner &scanner);

    Owned<Statement> parse();
    Signature parseSignature();

    void declare(const Signature &signature);

    const std::vector<SyntaxError> &errors();

  private:
    bool isAtEnd();
    bool check(const std::initializer_list<Token::Type> &types);
    Optional<Token> match(const std::initializer_list<Token::Type> &types);
    Optional<Token> matchWord();
    Token consume(Token::Type type, const std::string &error);
    Token consumeEnd(Token::Type type);
    Token consumeNewLine();
    Token consumeWord(const std::string &message = "expected a word");
    Token &scan();
    Token &advance();
    Token &peek();
    Token &previous();
    void synchronize();

    void checkpoint();
    void rewind();
    void commit();

    bool matchSignature(const Signature &signature, std::vector<Optional<Token>> &tokens,
                        std::vector<Owned<Expression>> &arguments);
    bool matchTerm(const Signature &signature, int index,
                   std::vector<Optional<Token>> &tokens,
                   std::vector<Owned<Expression>> &arguments);

#if defined(DEBUG)
    void _trace(const std::string &message);
#endif

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
    Scanner &_scanner;
    std::vector<SyntaxError> _errors;

    std::set<Signature> _functionDecls;
    std::vector<Variable> _variableDecls;

    std::vector<Token> _tokens;
    std::stack<size_t> _saved;
    size_t _index;

    bool _recording;
    bool _parsingRepeat;
};

CH_NAMESPACE_END
