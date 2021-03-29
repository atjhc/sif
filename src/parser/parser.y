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

#include "ast/Script.h"
#include "ast/Chunk.h"
#include "ast/Command.h"
#include "ast/Repeat.h"
#include "parser/Parser.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
#endif

using namespace chatter;
using namespace chatter::ast;

#include "parser/yy_shared.h"

%}

%error-verbose
%verbose
%locations
%expect 0

%union {
    Script          *script;
    Handler         *handler;
    Statement       *statement;
    StatementList   *statementList;
    Identifier      *identifier;
    IdentifierList  *identifierList;
    Expression      *expression;
    ExpressionList  *expressionList;
    Preposition     *preposition;
    Chunk           *chunk;
    Chunk::Type      chunkType;
}

%{

int yyerror(YYLTYPE*, yyscan_t, ParserContext&, const char *);

%}

// Virtual start tokens.
%token START_SCRIPT
%token START_STATEMENT
%token START_EXPRESSION

// Keywords
%token THE ON END FROM BY FUNCTION DO EXIT REPEAT TO COMMA GLOBAL NEXT PASS RETURN 
%token SEND WINDOW PROGRAM IF THEN ELSE FOREVER WITH UNTIL WHILE FOR DOWN TIMES 
%token NOT AN NO OR CONTAINS IS IN WITHIN OF FORM_FEED LINE_FEED UP AND EOL

// Commands
%token PUT GET ASK ADD SUBTRACT MULTIPLY DIVIDE

// Prepositions
%token INTO BEFORE AFTER

// Expressions
%token LPAREN RPAREN PLUS MINUS MULT DIV LT GT LTE GTE NEQ CARROT

// Constants
%token EMPTY FALSE QUOTE SPACE TAB TRUE ZERO ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN PI

// Ordinals
%token FIRST SECOND THIRD FOURTH FIFTH SIXTH SEVENTH EIGHTH NINTH TENTH LAST MIDDLE ANY

// Chunks
%token CHAR WORD LINE ITEM

%left OR AND
%left IS EQ NEQ
%left NOT
%left CONTAINS
%left LT GT LTE GTE 
%left CONCAT CONCAT_SPACE
%left PLUS MINUS
%left MULT DIV DIV_TRUNC MOD
%left CARROT

%right IDENTIFIER OF

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
%nterm <expression> expression functionCall ordinal constant
%nterm <chunk> chunk
%nterm <chunkType> chunkType
%nterm <expressionList> expressionList
%nterm <preposition> preposition

//%destructor { delete $$; $$ = nullptr; } IDENTIFIER INT_LITERAL STRING_LITERAL FLOAT_LITERAL
//%destructor { delete $$; $$ = nullptr; } <script> <handler> <identifier> <statement> <expression>
//%destructor { delete $$; $$ = nullptr; } <statementList> <expressionList> <identifierList>
//%destructor { delete $$; $$ = nullptr; } <chunk> <preposition>

%start start

%%

start
    : START_SCRIPT scriptList { 
        context.script = $2;
    }
    | START_STATEMENT statement {
        context.statement = $2;
    }
    | START_EXPRESSION expression {
        context.expression = $2;
    }
;

scriptList
    : script maybeEOL { 
        $$ = $1;
    }
    | EOL script maybeEOL {
        $$ = $2;
    }
    | EOL {
        $$ = nullptr;
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
        if ($2->name == $7->name) {
            $$ = new Handler(Handler::HandlerKind, $2, $3, $5);
            $$->location = @2.first;
        } else {
            $$ = nullptr;
            auto msg = "Expected " + $2->name + ", got " + $7->name;
            yyerror(&yylloc, scanner, context, msg.c_str());
        }
    }
    | FUNCTION messageKey identifierList EOL
        statementList
      END messageKey {
        if ($2->name == $7->name) {
            $$ = new Handler(Handler::FunctionKind, $2, $3, $5);
            $$->location = @2.first;
        } else {
            auto msg = "Expected " + $2->name + ", got " + $7->name;
            yyerror(&yylloc, scanner, context, msg.c_str());
        }
    }
;

// TODO: I'm a little suspect of this.
identifierList
    : /* empty */ {
        $$ = nullptr;
    } 
    | IDENTIFIER {
        if ($1) {
            $$ = new IdentifierList();
            $$->add($1);
        } else {
            $$ = nullptr;
        }
    }
    | identifierList COMMA IDENTIFIER {
        if ($1 && $3) {
            $$ = $1;
            $$->add($3);
        } else {
            $$ = nullptr;
        }
    }
;

statementList
    : /* empty */ {
        $$ = new StatementList();
    }
    | statement { 
        if ($1) {
            $$ = new StatementList();
            $$->add($1);
        } else {
            $$ = nullptr;
        }
    }
    | statementList EOL statement {
        if ($3) {
            if ($1) {
                $$ = $1;
            } else {
                $$ = new StatementList();
            }
            $$->add($3);
        } else {
            $$ = nullptr;
        }
    }
    | statementList EOL {
        $$ = $1;
    }
;

statement
    : keywordStatement { 
        $$ = $1;
    } 
    | commandStatement {
        $$ = $1;
    }
;

messageKey
    : IDENTIFIER {
        $$ = $1;
    }
    | PUT {
        $$ = new Identifier("put");
        $$->location = @1.first;
    }
    | GET {
        $$ = new Identifier("get");
        $$->location = @1.first;
    }
    | ASK {
        $$ = new Identifier("ask");
        $$->location = @1.first;
    }
    | ADD {
        $$ = new Identifier("add");
        $$->location = @1.first;
    }
    | SUBTRACT {
        $$ = new Identifier("subtract");
        $$->location = @1.first;
    }
    | MULTIPLY {
        $$ = new Identifier("multiply");
        $$->location = @1.first;
    }
    | DIVIDE {
        $$ = new Identifier("divide");
        $$->location = @1.first;
    }
;

keywordStatement
    : EXIT REPEAT { 
        $$ = new ExitRepeat();
        $$->location = @1.first;
    }
    | NEXT REPEAT { 
        $$ = new NextRepeat();
        $$->location = @1.first;
    }
    | EXIT messageKey {
        $$ = new Exit($2);
        $$->location = @1.first;
    }
    | PASS messageKey { 
        $$ = new Pass($2);
        $$->location = @1.first;
    }
    | GLOBAL identifierList {
        $$ = new Global($2);
        $$->location = @1.first;
    }
    | RETURN expression {
        $$ = new Return($2);
        $$->location = @1.first;
    }
    | ifBlock { 
        $$ = $1;
    }
    | repeatBlock {
        $$ = $1;
    }
;

commandStatement
    : PUT expression {
        $$ = new Put($2, nullptr, nullptr);
        $$->location = @1.first;
    }
    | PUT expression preposition IDENTIFIER {
        $$ = new Put($2, $3, $4);
        $$->location = @1.first;
    }
    | GET expression {
        $$ = new Get($2);
        $$->location = @1.first;
    }
    | ASK expression {
        $$ = new Ask($2);
        $$->location = @1.first;
    }
    | ADD expression TO IDENTIFIER {
        $$ = new Add($2, $4);
        $$->location = @1.first;
    }
    | SUBTRACT expression FROM IDENTIFIER {
        $$ = new Subtract($2, $4);
        $$->location = @1.first;
    }
    | MULTIPLY IDENTIFIER BY expression {
        $$ = new Multiply($4, $2);
        $$->location = @1.first;
    }
    | DIVIDE IDENTIFIER BY expression {
        $$ = new Divide($4, $2);
        $$->location = @1.first;
    }
    | IDENTIFIER {
        $$ = new Command($1, nullptr);
        $$->location = @1.first;
    }
    | IDENTIFIER expressionList {
        $$ = new Command($1, $2);
        $$->location = @1.first;
    }
;

ifBlock
    : IF expression maybeEOL THEN statement {
        if ($2 && $5) {
            $$ = new If($2, new StatementList($5), nullptr);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | IF expression maybeEOL THEN EOL statementList END IF {
        if ($2 && $6) {
            $$ = new If($2, $6, nullptr);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    // TODO: Add missing IF/ELSE construct:
    //   if expression then statement
    //   else statement
    | IF expression maybeEOL THEN statement elseBlock {
        if ($2 && $5 && $6) {
            $$ = new If($2, new StatementList($5), $6);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | IF expression maybeEOL THEN EOL statementList elseBlock {
        if ($2 && $6 && $7) {
            $$ = new If($2, $6, $7);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
;

elseBlock
    : ELSE statement {
        if ($2) {
            $$ = new StatementList($2);
            $$->location = @2.first;
        } else {
            $$ = nullptr;
        }
    }
    | ELSE EOL statementList END IF {
        $$ = $3;
    }
;

repeatBlock
    : repeatForever {
        $$ = $1;
    }
    | repeatCount {
        $$ = $1;
    }
    | repeatCondition {
        $$ = $1;
    }
    | repeatRange {
        $$ = $1;
    }
;

repeatForever
    : REPEAT maybeForever EOL 
        statementList 
      END REPEAT {
        $$ = new Repeat($4);
        $$->location = @1.first;
    }
;

repeatCount
    : REPEAT maybeFor expression maybeTimes EOL 
        statementList 
      END REPEAT {
        if ($3) {
            $$ = new RepeatCount($3, $6);
            $$->location = @1.first;
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
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | REPEAT UNTIL expression EOL 
        statementList 
      END REPEAT {
        if ($3) {
            $$ = new RepeatCondition($3, false, $5);
            $$->location = @1.first;
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
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | REPEAT WITH IDENTIFIER EQ expression DOWN TO expression EOL
        statementList
      END REPEAT {
        if ($5 && $8) {
            $$ = new RepeatRange($3, $5, $8, false, $10);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
;

maybeForever
    : /* empty */
    | FOREVER
;

maybeFor
    : /* empty */
    | FOR
;

maybeTimes
    : /* empty */
    | TIMES
;

expression
    : LPAREN expression RPAREN {
        $$ = $2;
    }
    | expression PLUS expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Plus, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression MINUS expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Minus, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression MULT expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Multiply, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression DIV expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Divide, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | expression IS expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Equal, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression EQ expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Equal, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression MOD expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Mod, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression NEQ expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::NotEqual, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression OR expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Or, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression AND expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::And, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression IS IN expression %prec AND {
        if ($1 && $4) {
            $$ = new BinaryOp(BinaryOp::IsIn, $1, $4);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CONTAINS expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Contains, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression LT expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::LessThan, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression GT expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::GreaterThan, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | expression LTE expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::LessThanOrEqual, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression GTE expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::GreaterThanOrEqual, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CONCAT expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Concat, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CONCAT_SPACE expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::ConcatWithSpace, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CARROT expression {
        if ($1 && $3) {
            $$ = new BinaryOp(BinaryOp::Exponent, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | chunk OF expression {
        if ($1) {
            $$ = $1;
            $1->expression = $3;
        } else {
            $$ = nullptr;
        }
    }
    | functionCall {
        $$ = $1;
    } 
    | MINUS expression {
        if ($2) {
            $$ = new Minus($2);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | NOT expression {
        if ($2) {
            $$ = new Not($2);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | constant {
        $$ = $1;
    }
    | IDENTIFIER {
        $$ = $1;
    }
    | INT_LITERAL {
        $$ = $1;
    }
    | FLOAT_LITERAL {
        $$ = $1;
    }
    | STRING_LITERAL {
        $$ = $1;
    }
;

functionCall
    : THE IDENTIFIER {
        $$ = new FunctionCall($2, nullptr);
        $$->location = @2.first;
    }
    | THE IDENTIFIER OF expression {
        if ($4) {
            $$ = new FunctionCall($2, new ExpressionList($4));
            $$->location = @2.first;
        } else {
            $$ = nullptr;
        }
    }
    | IDENTIFIER LPAREN expressionList RPAREN {
        if ($3) {
            $$ = new FunctionCall($1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | IDENTIFIER LPAREN RPAREN {
        $$ = new FunctionCall($1, nullptr);
        $$->location = @1.first;
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
        if ($3) {
            $$ = $1;
            $$->add($3);
        } else {
            $$ = nullptr;
        }
    }
;

preposition
    : INTO {
        $$ = new Preposition(Preposition::Into);
    }
    | BEFORE {
        $$ = new Preposition(Preposition::Before);
    }
    | AFTER {
        $$ = new Preposition(Preposition::After);
    }
;

chunk
    : maybeThe ordinal chunkType {
        $$ = new RangeChunk($3, $2, nullptr);
        $$->location = @2.first;
    }
    | chunkType expression {
        $$ = new RangeChunk($1, $2, nullptr);
        $$->location = @1.first;
    }
    | chunkType expression TO expression {
        $$ = new RangeChunk($1, $2, $4);
        $$->location = @1.first;
    }
    | ANY chunkType {
        $$ = new AnyChunk($2);
        $$->location = @1.first;
    }
    | maybeThe MIDDLE chunkType {
        $$ = new MiddleChunk($3);
        $$->location = @2.first;
    }
    | maybeThe LAST chunkType {
        $$ = new LastChunk($3);
        $$->location = @2.first;
    }
;

chunkType
    : CHAR {
        $$ = Chunk::Char;
    }
    | WORD {
        $$ = Chunk::Word;
    }
    | ITEM {
        $$ = Chunk::Item;
    }
    | LINE {
        $$ = Chunk::Line;
    }
;

ordinal
    : FIRST {
        $$ = new IntLiteral(1);
    }
    | SECOND {
        $$ = new IntLiteral(2);
    }
    | THIRD {
        $$ = new IntLiteral(3);
    }
    | FOURTH {
        $$ = new IntLiteral(4);
    }
    | FIFTH {
        $$ = new IntLiteral(5);
    }
    | SIXTH {
        $$ = new IntLiteral(6);
    }
    | SEVENTH {
        $$ = new IntLiteral(7);
    }
    | EIGHTH {
        $$ = new IntLiteral(8);
    }
    | NINTH {
        $$ = new IntLiteral(9);
    }
    | TENTH {
        $$ = new IntLiteral(10);
    }
;

constant
    : TRUE {
        $$ = new StringLiteral("true");
    }
    | FALSE {
        $$ = new StringLiteral("false");
    }
    | EMPTY {
        $$ = new StringLiteral("");
    }
    | RETURN {
        $$ = new StringLiteral("\n");
    }
    | TAB {
        $$ = new StringLiteral("\t");
    }
    | SPACE {
        $$ = new StringLiteral(" ");
    }
    | QUOTE {
        $$ = new StringLiteral("\"");
    }
    | ZERO {
        $$ = new IntLiteral(0);
    }
    | ONE {
        $$ = new IntLiteral(1);
    }
    | TWO {
        $$ = new IntLiteral(2);
    }
    | THREE {
        $$ = new IntLiteral(3);
    }
    | FOUR {
        $$ = new IntLiteral(4);
    }
    | FIVE {
        $$ = new IntLiteral(5);
    }
    | SIX {
        $$ = new IntLiteral(6);
    }
    | SEVEN {
        $$ = new IntLiteral(7);
    }
    | EIGHT {
        $$ = new IntLiteral(8);
    }
    | NINE {
        $$ = new IntLiteral(9);
    }
    | TEN {
        $$ = new IntLiteral(10);
    }
    | PI {
        $$ = new FloatLiteral(M_PI);
    }
;

maybeThe
    : /* empty */
    | THE
;

maybeEOL
    : /* empty */ 
    | EOL
;

%%

int yyerror(YYLTYPE *yylloc, yyscan_t scanner, ParserContext &context, const char *msg) {
    context.error(yylloc->first, msg);
    return 0;
}
