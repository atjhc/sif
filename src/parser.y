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

%pure-parser
%lex-param   { scanner }
%lex-param   { context }
%parse-param { yyscan_t scanner }
%parse-param { ParserContext &context }
%{

#include <string>
#include <iostream>
#include <cstdlib>

#include "ast/ast.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
#endif

using namespace hypertalk::ast;

%}

%error-verbose
%verbose
%debug

%union {
    Script          *script;
    Handler         *handler;
    Statement       *statement;
    StatementList   *statementList;
    Identifier      *identifier;
    IdentifierList  *identifierList;
    Expression      *expression;
    ExpressionList  *expressionList;
}

%{

int yylex(YYSTYPE*, yyscan_t, ParserContext&);
int yyerror(yyscan_t, const ParserContext&, const char *);

%}

%token THE ON END FUNCTION DO EXIT REPEAT TO COMMA GLOBAL NEXT PASS RETURN SEND WINDOW PROGRAM IF THEN ELSE FOREVER WITH UNTIL WHILE FOR DOWN TIMES NOT AN NO OR CONTAINS IS IN WITHIN OF EMPTY FALSE FORM_FEED LINE_FEED PI QUOTE SPACE TAB TRUE UP ZERO ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN AND EOL
%token PUT GET
%token INTO BEFORE AFTER
%token LPAREN RPAREN PLUS MINUS MULT DIVIDE LT GT LTE GTE NEQ

%left OR AND
%left IS EQ NEQ /* IS NOT */
%left CONTAINS /* IS IN */
%left LT GT LTE GTE 
%left CONCAT CONCAT_SPACE
%left PLUS MINUS
%left MULT DIV DIV_TRUNC MOD

%left IDENTIFIER OF

%nonassoc THEN
%nonassoc ELSE

%token <expression> FLOAT_LITERAL INT_LITERAL STRING_LITERAL
%token <identifier> IDENTIFIER

%nterm <script> script scriptList
%nterm <handler> handler
%nterm <identifier> messageKey
%nterm <identifierList> identifierList
%nterm <statementList> statementList elseBlock
%nterm <statement> statement ifBlock keywordStatement commandStatement
%nterm <statement> repeatBlock repeatForever repeatCount repeatCondition repeatRange
%nterm <expression> expression condition functionCall
%nterm <expressionList> expressionList

%destructor { delete $$; } IDENTIFIER
%start start

// TODO: Remove this once the existing conflicts are resolved.
%expect 28

%%

start
    : scriptList { 
        context.script = $1 
    }
;

scriptList
    : script maybeEOL { 
        $$ = $1 
    }
    | EOL script maybeEOL {
        $$ = $2
    }
    | EOL {
        $$ = nullptr
    }
;

script
    : handler { 
        $$ = new Script();
        if ($1) {
            $$->add($1);
        } 
    }
    | script EOL handler { 
        $$ = $1;
        if ($3) {
            $$->add($3);
        }
    }
;

handler
    : ON messageKey identifierList EOL
        statementList
      END messageKey {
        if ($2->_name == $7->_name) {
            $$ = new Handler(Handler::HandlerKind, $2, $3, $5);
        } else {
            $$ = nullptr;
        }
    }
    | FUNCTION messageKey identifierList EOL
        statementList
      END messageKey {
        if ($2->_name == $7->_name) {
            $$ = new Handler(Handler::FunctionKind, $2, $3, $5);
        } else {
            $$ = nullptr;
        }
    }
;

// TODO: I'm a little suspect of this.
identifierList
    : /* empty */ {
        $$ = nullptr
    } 
    | IDENTIFIER {
        $$ = new IdentifierList();
        $$->add($1);
    }
    | identifierList COMMA IDENTIFIER {
        $$ = $1;
        $$->add($3);
    }
;

statementList
    : /* empty */ {
        $$ = new StatementList();
    }
    | statement { 
        $$ = new StatementList();
        if ($1) {
            $$->add($1);
        } 
    }
    | statementList EOL statement {
        if ($1) {
            $$ = $1;
        } else {
            $$ = new StatementList();
        }
        if ($3) {
            $$->add($3);
        }
    }
    | statementList EOL {
        $$ = $1
    }
;

statement
    : keywordStatement { 
        $$ = $1
    } 
    | commandStatement {
        $$ = $1
    }
;

messageKey
    : IDENTIFIER {
        $$ = $1
    }
;

keywordStatement
    : EXIT REPEAT { 
        $$ = new ExitRepeat()
    }
    | EXIT messageKey {
        $$ = new Exit($2)
    }
    | PASS messageKey { 
        $$ = new Pass($2)
    }
    | GLOBAL identifierList {
        $$ = new Global($2)
    }
    | RETURN expression {
        $$ = new Return($2)
    }
    | ifBlock { 
        $$ = $1
    }
    | repeatBlock {
        $$ = $1
    }
;

commandStatement
    : PUT expression {
        $$ = new Put($2, nullptr)
    }
    // TODO: container support
    | PUT expression preposition IDENTIFIER {
        $$ = new Put($2, $4)
    }
    | GET expression {
        $$ = new Get($2)
    }
;

ifBlock
    : IF condition maybeEOL THEN statement {
        if ($2 && $5) {
            $$ = new If($2, new StatementList($5), nullptr);
        } else {
            $$ = nullptr;
        }
    }
    | IF condition maybeEOL THEN EOL statementList END IF {
        if ($2 && $6) {
            $$ = new If($2, $6, nullptr);
        } else {
            $$ = nullptr;
        }
    }
    | IF condition maybeEOL THEN statement elseBlock {
        if ($2 && $5 && $6) {
            $$ = new If($2, new StatementList($5), $6);
        } else {
            $$ = nullptr;
        }
    } 
    | IF condition maybeEOL THEN EOL statementList elseBlock {
        if ($2 && $6 && $7) {
            $$ = new If($2, $6, $7);
        } else {
            $$ = nullptr;
        }
    }
;

elseBlock
    : ELSE statement {
        if ($2) {
            $$ = new StatementList($2);
        } else {
            $$ = nullptr;
        }
    }
    | ELSE EOL statementList END IF {
        $$ = $3
    }
;

repeatBlock
    : repeatForever {
        $$ = $1
    }
    | repeatCount {
        $$ = $1
    }
    | repeatCondition {
        $$ = $1
    }
    | repeatRange {
        $$ = $1
    }
;

repeatForever
    : REPEAT maybeForever EOL 
        statementList 
      END REPEAT {
        $$ = new Repeat($4)
    }
;

repeatCount
    : REPEAT expression maybeTimes EOL 
        statementList 
      END REPEAT {
        if ($2) {
            $$ = new RepeatCount($2, $5);
        } else {
            $$ = nullptr;
        }
    }
;

repeatCondition
    : REPEAT WHILE expression EOL 
        statementList 
      END REPEAT {
        if ($3) {
            $$ = new RepeatCondition($3, true, $5);
        } else {
            $$ = nullptr;
        }
    }
    | REPEAT UNTIL expression EOL 
        statementList 
      END REPEAT {
        if ($3) {
            $$ = new RepeatCondition($3, false, $5);
        } else {
            $$ = nullptr;
        }
    }
;

repeatRange
    : REPEAT WITH IDENTIFIER EQ expression TO expression EOL
        statementList
      END REPEAT {
        if ($5 && $7) {
            $$ = new RepeatRange($3, $5, $7, true, $9);
        } else {
            $$ = nullptr;
        }
    }
    | REPEAT WITH IDENTIFIER EQ expression DOWN TO expression EOL
        statementList
      END REPEAT {
        if ($5 && $8) {
            $$ = new RepeatRange($3, $5, $8, false, $10);
        } else {
            $$ = nullptr;
        }
    }
;

maybeForever
    : /* empty */
    | FOREVER
;

maybeTimes
    : /* empty */
    | TIMES
;

condition
    : expression {
        $$ = $1
    }
;

expression
    : LPAREN expression RPAREN {
        $$ = $2
    }
    | expression PLUS expression {
        $$ = new BinaryOp(BinaryOp::Plus, $1, $3)
    }
    | expression MINUS expression {
        $$ = new BinaryOp(BinaryOp::Minus, $1, $3)
    }
    | expression MULT expression {
        $$ = new BinaryOp(BinaryOp::Multiply, $1, $3)
    }
    | expression DIV expression {
        $$ = new BinaryOp(BinaryOp::Divide, $1, $3)
    } 
    | expression IS expression {
        $$ = new BinaryOp(BinaryOp::Equal, $1, $3)
    }
    | expression EQ expression {
        $$ = new BinaryOp(BinaryOp::Equal, $1, $3)
    }
    // TODO: Fix shift/reduce conflict
    | expression notEqualTo expression {
        $$ = new BinaryOp(BinaryOp::NotEqual, $1, $3)
    }
    | expression OR expression {
        $$ = new BinaryOp(BinaryOp::Or, $1, $3)
    }
    | expression AND expression {
        $$ = new BinaryOp(BinaryOp::And, $1, $3)
    }
    // TODO: Fix shift/reduce conflict
    | expression IS IN expression {
        $$ = new BinaryOp(BinaryOp::IsIn, $1, $4)
    }
    | expression CONTAINS expression {
        $$ = new BinaryOp(BinaryOp::Contains, $1, $3)
    }
    | expression LT expression {
        $$ = new BinaryOp(BinaryOp::LessThan, $1, $3)
    }
    | expression GT expression {
        $$ = new BinaryOp(BinaryOp::GreaterThan, $1, $3)
    } 
    | expression LTE expression {
        $$ = new BinaryOp(BinaryOp::LessThanOrEqual, $1, $3)
    }
    | expression GTE expression {
        $$ = new BinaryOp(BinaryOp::GreaterThanOrEqual, $1, $3)
    }
    | functionCall {
        $$ = $1
    } 
    | IDENTIFIER {
        $$ = $1
    }
    | INT_LITERAL {
        $$ = $1
    }
    | FLOAT_LITERAL {
        $$ = $1
    }
    | STRING_LITERAL {
        $$ = $1
    }
;

functionCall
    : THE IDENTIFIER {
        $$ = new FunctionCall($2, nullptr)
    }
    | THE IDENTIFIER OF expression {
        $$ = new FunctionCall($2, new ExpressionList($4))
    }
    | IDENTIFIER LPAREN expressionList RPAREN {
        $$ = new FunctionCall($1, $3)
    }
    // TODO: Not positive if I need this.
    | IDENTIFIER LPAREN RPAREN {
        $$ = new FunctionCall($1, nullptr)
    }
;

expressionList
    : expression {
        $$ = new ExpressionList();
        if ($1) {
            $$->add($1);
        }
    }
    | expressionList COMMA expression {
        $$ = $1;
        if ($3) {
            $$->add($3);
        }
    }
;

preposition
    : INTO
    | BEFORE
    | AFTER
;

notEqualTo
    : NEQ
    | IS NOT
;

maybeEOL
    : /* empty */ 
    | EOL
;

%%

#define YYDEBUG 1

using namespace hypertalk::ast;

int yyerror(yyscan_t scanner, const ParserContext &context, const char *msg) {
    auto lineNumber = context.currentLocation.lineNumber;
    auto position = context.currentLocation.position;
    std::cout << context.fileName << ":" << lineNumber << ":" << position << ": ";
    std::cout << msg << std::endl;
    return 0;
}
