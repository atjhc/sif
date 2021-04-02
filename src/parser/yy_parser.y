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

%require "3.7"

%{

#include <string>
#include <iostream>
#include <cstdlib>

#include "Common.h"
#include "parser/Parser.h"

using namespace chatter;
using namespace chatter::ast;

%}

%code provides {
    #define YY_DECL \
        int yylex(yy::parser::semantic_type* yylval, \
                  yy::parser::location_type* yylloc, \
                  void *yyscanner,                   \
                  chatter::ParserContext &context)
    extern YY_DECL;
}

//%define api.pure
//%language "c++"
%skeleton "lalr1.cc"

%param { yyscan_t scanner } { chatter::ParserContext &ctx }

%define api.location.type { ParseLocation }
%code requires { 
    #include "parser/yy_shared.h"
    #include "ast/Script.h"
    #include "ast/Chunk.h"
    #include "ast/Command.h"
    #include "ast/Repeat.h"
}

%define parse.error detailed
%define api.value.type variant
%define api.token.raw
//%define api.value.automove
//%define api.token.constructor

%verbose
%locations
%expect 0

// Virtual start tokens.
%token START_SCRIPT
%token START_STATEMENT
%token START_EXPRESSION

// Keywords
%token ON END FROM BY FUNCTION DO EXIT REPEAT TO COMMA GLOBAL NEXT PASS RETURN 
%token WINDOW PROGRAM IF THEN ELSE FOREVER WITH UNTIL WHILE FOR DOWN TIMES 
%token NOT THE AN NO OR CONTAINS IS IN WITHIN OF FORM_FEED LINE_FEED UP AND EOL

// Commands
%token PUT GET ASK ADD SUBTRACT MULTIPLY DIVIDE

// Prepositions
%token INTO BEFORE AFTER

// Expressions
%token LPAREN RPAREN PLUS MINUS MULT DIV LT GT LTE GTE NEQ CARROT

// Constants
%token EMPTY FALSE QUOTE SPACE TAB TRUE PI
%token ZERO ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN

// Ordinals
%token FIRST SECOND THIRD FOURTH FIFTH SIXTH SEVENTH EIGHTH NINTH TENTH
%token LAST MIDDLE ANY

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

%token <Owned<Expression>> FLOAT_LITERAL INT_LITERAL STRING_LITERAL
%token <Owned<Identifier>> IDENTIFIER

%nterm <Owned<Script>> script
%nterm <Owned<Handler>> handler
%nterm <Owned<Identifier>> messageKey
%nterm <Owned<IdentifierList>> maybeIdentifierList identifierList
%nterm <Owned<StatementList>> statementList maybeStatementList elseBlock
%nterm <Owned<Statement>> statement ifStatement keywordStatement commandStatement
%nterm <Owned<Statement>> repeatStatement repeatForever repeatCount repeatCondition repeatRange
%nterm <Owned<Expression>> expression ifCondition functionCall ordinal constant
%nterm <Owned<Chunk>> chunk
%nterm <Chunk::Type> chunkType
%nterm <Owned<ExpressionList>> expressionList
%nterm <Owned<Preposition>> preposition

//%destructor { delete $$; $$ = nullptr; } IDENTIFIER INT_LITERAL STRING_LITERAL FLOAT_LITERAL
//%destructor { delete $$; $$ = nullptr; } maybeStatementList handler statementList statement messageKey
//%destructor { delete $$; $$ = nullptr; } chunk preposition ordinal constant functionCall expression expressionList

%start start

%%

start
    : START_SCRIPT script { 
        ctx.script = std::move($2);
    }
    | START_STATEMENT statement {
        ctx.statement = std::move($2);
    }
    | START_EXPRESSION expression {
        ctx.expression = std::move($2);
    }
;

script
    : handler {
        $$ = MakeOwned<Script>();
        if ($1) {
            $$->add($1);
        } 
    }
    | script EOL handler { 
        $$ = std::move($1);
        if ($3) {
            $$->add($3);
        }
    }
;

handler
    : /* empty */ {
        $$ = nullptr;
    }
    | ON messageKey maybeIdentifierList EOL
        maybeStatementList
      END messageKey {
        if ($2 && $7) {
            if ($2->name == $7->name) {
                $$ = MakeOwned<Handler>(Handler::HandlerKind, $2, $3, $5);
                $$->location = @2.first;
            } else {
                auto msg = "Expected " + $2->name + ", got " + $7->name;
                error(@$, msg.c_str());
            }
        } else {
            $$ = nullptr;
        }
    }
    | FUNCTION messageKey maybeIdentifierList EOL
        maybeStatementList
      END messageKey {
        if ($2 && $7) {
            if ($2->name == $7->name) {
                $$ = MakeOwned<Handler>(Handler::FunctionKind, $2, $3, $5);
                $$->location = @2.first;
            } else {
                auto msg = "Expected " + $2->name + ", got " + $7->name;
                error(@$, msg.c_str());
            }
        } else {
            $$ = nullptr;
        }
    }
;

maybeIdentifierList
    : /* empty */ {
        $$ = nullptr;
    }
    | identifierList {
        $$ = std::move($1);
    }
;

identifierList
    : IDENTIFIER {
        if ($1) {
            $$ = MakeOwned<IdentifierList>();
            $$->add($1);
        } else {
            $$ = nullptr;
        }
    }
    | identifierList COMMA IDENTIFIER {
        if ($1 && $3) {
            $$ = std::move($1);
            $$->add($3);
        } else {
            $$ = nullptr;
        }
    }
;

maybeStatementList
    : /* empty */ {
        $$ = nullptr;    
    }
    | statementList {
        $$ = std::move($1);
    }
;

statementList
    : statement { 
        if ($1) {
            $$ = MakeOwned<StatementList>();
            $$->add($1);
        } else {
            $$ = nullptr;
        }
    }
    | statementList EOL statement {
        if ($3) {
            $$ = std::move($1);
            $$->add($3);
        } else {
            $$ = nullptr;
        }
    }
    | statementList EOL {
        $$ = std::move($1);
    }
;

statement
    : keywordStatement { 
        $$ = std::move($1);
    } 
    | commandStatement {
        $$ = std::move($1);
    }
;

messageKey
    : IDENTIFIER {
        $$ = std::move($1);
    }
    | PUT {
        $$ = MakeOwned<Identifier>("put");
        $$->location = @1.first;
    }
    | GET {
        $$ = MakeOwned<Identifier>("get");
        $$->location = @1.first;
    }
    | ASK {
        $$ = MakeOwned<Identifier>("ask");
        $$->location = @1.first;
    }
    | ADD {
        $$ = MakeOwned<Identifier>("add");
        $$->location = @1.first;
    }
    | SUBTRACT {
        $$ = MakeOwned<Identifier>("subtract");
        $$->location = @1.first;
    }
    | MULTIPLY {
        $$ = MakeOwned<Identifier>("multiply");
        $$->location = @1.first;
    }
    | DIVIDE {
        $$ = MakeOwned<Identifier>("divide");
        $$->location = @1.first;
    }
;

keywordStatement
    : EXIT REPEAT { 
        $$ = MakeOwned<ExitRepeat>();
        $$->location = @1.first;
    }
    | NEXT REPEAT { 
        $$ = MakeOwned<NextRepeat>();
        $$->location = @1.first;
    }
    | EXIT messageKey {
        $$ = MakeOwned<Exit>($2);
        $$->location = @1.first;
    }
    | PASS messageKey { 
        $$ = MakeOwned<Pass>($2);
        $$->location = @1.first;
    }
    | GLOBAL identifierList {
        $$ = MakeOwned<Global>($2);
        $$->location = @1.first;
    }
    | RETURN expression {
        $$ = MakeOwned<Return>($2);
        $$->location = @1.first;
    }
    | ifStatement {
        $$ = std::move($1);
    }
    | repeatStatement {
        $$ = std::move($1);
    }
;

commandStatement
    : PUT expression {
        $$ = MakeOwned<Put>($2);
        $$->location = @1.first;
    }
    | PUT expression preposition IDENTIFIER {
        $$ = MakeOwned<Put>($2, $3, $4);
        $$->location = @1.first;
    }
    | GET expression {
        $$ = MakeOwned<Get>($2);
        $$->location = @1.first;
    }
    | ASK expression {
        $$ = MakeOwned<Ask>($2);
        $$->location = @1.first;
    }
    | ADD expression TO IDENTIFIER {
        $$ = MakeOwned<Add>($2, $4);
        $$->location = @1.first;
    }
    | SUBTRACT expression FROM IDENTIFIER {
        $$ = MakeOwned<Subtract>($2, $4);
        $$->location = @1.first;
    }
    | MULTIPLY IDENTIFIER BY expression {
        $$ = MakeOwned<Multiply>($4, $2);
        $$->location = @1.first;
    }
    | DIVIDE IDENTIFIER BY expression {
        $$ = MakeOwned<Divide>($4, $2);
        $$->location = @1.first;
    }
    | IDENTIFIER {
        $$ = MakeOwned<Command>($1);
        $$->location = @1.first;
    }
    | IDENTIFIER expressionList {
        $$ = MakeOwned<Command>($1, $2);
        $$->location = @1.first;
    }
;

ifStatement
    : ifCondition THEN statement {
        if ($1 && $3) {
            $$ = MakeOwned<If>($1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | ifCondition THEN EOL statementList END IF {
        if ($1 && $4) {
            $$ = MakeOwned<If>($1, $4);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | ifCondition THEN statement elseBlock {
        if ($1 && $3 && $4) {
            $$ = MakeOwned<If>($1, $3, $4);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | ifCondition THEN EOL statementList elseBlock {
        if ($1 && $4 && $5) {
            $$ = MakeOwned<If>($1, $4, $5);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
// TODO: The following construction is ambiguous, but valid
//       in HyperTalk. For now, Chatter does not allow these.
/*
    | ifCondition THEN statement EOL elseBlock {
        if ($1 && $4 && $5) {
            $$ = new If($1, $4, $5);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
*/
;

elseBlock
    : ELSE statement {
        if ($2) {
            $$ = MakeOwned<StatementList>($2);
            $$->location = @2.first;
        } else {
            $$ = nullptr;
        }
    }
    | ELSE EOL statementList END IF {
        $$ = std::move($3);
    }
;

ifCondition
    : IF expression {
        $$ = std::move($2);
    }
    | IF expression EOL {
        $$ = std::move($2);
    }
;

repeatStatement
    : repeatForever {
        $$ = std::move($1);
    }
    | repeatCount {
        $$ = std::move($1);
    }
    | repeatCondition {
        $$ = std::move($1);
    }
    | repeatRange {
        $$ = std::move($1);
    }
;

repeatForever
    : REPEAT maybeForever EOL 
        statementList 
      END REPEAT {
        if ($4) {
            $$ = MakeOwned<Repeat>($4);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
;

repeatCount
    : REPEAT maybeFor expression maybeTimes EOL 
        statementList 
      END REPEAT {
        if ($3) {
            $$ = MakeOwned<RepeatCount>($3, $6);
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
            $$ = MakeOwned<RepeatCondition>($3, true, $5);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | REPEAT UNTIL expression EOL 
        statementList 
      END REPEAT {
        if ($3) {
            $$ = MakeOwned<RepeatCondition>($3, false, $5);
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
            $$ = MakeOwned<RepeatRange>($3, $5, $7, true, $9);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | REPEAT WITH IDENTIFIER EQ expression DOWN TO expression EOL
        statementList
      END REPEAT {
        if ($5 && $8) {
            $$ = MakeOwned<RepeatRange>($3, $5, $8, false, $10);
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
        $$ = std::move($2);
    }
    | expression PLUS expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Plus, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression MINUS expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Minus, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression MULT expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Multiply, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression DIV expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Divide, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | expression IS expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Equal, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression EQ expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Equal, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression MOD expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Mod, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression NEQ expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::NotEqual, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression OR expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Or, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression AND expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::And, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression IS IN expression %prec AND {
        if ($1 && $4) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::IsIn, $1, $4);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CONTAINS expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Contains, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression LT expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::LessThan, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression GT expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::GreaterThan, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | expression LTE expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::LessThanOrEqual, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression GTE expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::GreaterThanOrEqual, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CONCAT expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Concat, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CONCAT_SPACE expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::ConcatWithSpace, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | expression CARROT expression {
        if ($1 && $3) {
            $$ = MakeOwned<BinaryOp>(BinaryOp::Exponent, $1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | chunk OF expression {
        if ($1 && $3) {
            $1->expression = std::move($3);
            $$ = std::move($1);
        } else {
            $$ = nullptr;
        }
    }
    | functionCall {
        $$ = std::move($1);
    } 
    | MINUS expression {
        if ($2) {
            $$ = MakeOwned<Minus>($2);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    } 
    | NOT expression {
        if ($2) {
            $$ = MakeOwned<Not>($2);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | constant {
        $$ = std::move($1);
    }
    | IDENTIFIER {
        $$ = std::move($1);
    }
    | INT_LITERAL {
        $$ = std::move($1);
    }
    | FLOAT_LITERAL {
        $$ = std::move($1);
    }
    | STRING_LITERAL {
        $$ = std::move($1);
    }
;

functionCall
    : THE IDENTIFIER {
        $$ = MakeOwned<FunctionCall>($2);
        $$->location = @2.first;
    }
    | THE IDENTIFIER OF expression {
        if ($4) {
            $$ = MakeOwned<FunctionCall>($2, $4);
            $$->location = @2.first;
        } else {
            $$ = nullptr;
        }
    }
    | IDENTIFIER LPAREN expressionList RPAREN {
        if ($3) {
            $$ = MakeOwned<FunctionCall>($1, $3);
            $$->location = @1.first;
        } else {
            $$ = nullptr;
        }
    }
    | IDENTIFIER LPAREN RPAREN {
        $$ = MakeOwned<FunctionCall>($1);
        $$->location = @1.first;
    }
;

expressionList
    : expression {
        if ($1) {
            $$ = MakeOwned<ExpressionList>();
            $$->add($1);
        } else {
            $$ = nullptr;
        }
    }
    | expressionList COMMA expression {
        if ($1 && $3) {
            $$ = std::move($1);
            $$->add($3);
        } else {
            $$ = nullptr;
        }
    }
;

preposition
    : INTO {
        $$ = MakeOwned<Preposition>(Preposition::Into);
    }
    | BEFORE {
        $$ = MakeOwned<Preposition>(Preposition::Before);
    }
    | AFTER {
        $$ = MakeOwned<Preposition>(Preposition::After);
    }
;

chunk
    : maybeThe ordinal chunkType {
        $$ = MakeOwned<RangeChunk>($3, $2);
        $$->location = @2.first;
    }
    | chunkType expression {
        $$ = MakeOwned<RangeChunk>($1, $2);
        $$->location = @1.first;
    }
    | chunkType expression TO expression {
        $$ = MakeOwned<RangeChunk>($1, $2, $4);
        $$->location = @1.first;
    }
    | ANY chunkType {
        $$ = MakeOwned<AnyChunk>($2);
        $$->location = @1.first;
    }
    | maybeThe MIDDLE chunkType {
        $$ = MakeOwned<MiddleChunk>($3);
        $$->location = @2.first;
    }
    | maybeThe LAST chunkType {
        $$ = MakeOwned<LastChunk>($3);
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
        $$ = MakeOwned<IntLiteral>(1);
    }
    | SECOND {
        $$ = MakeOwned<IntLiteral>(2);
    }
    | THIRD {
        $$ = MakeOwned<IntLiteral>(3);
    }
    | FOURTH {
        $$ = MakeOwned<IntLiteral>(4);
    }
    | FIFTH {
        $$ = MakeOwned<IntLiteral>(5);
    }
    | SIXTH {
        $$ = MakeOwned<IntLiteral>(6);
    }
    | SEVENTH {
        $$ = MakeOwned<IntLiteral>(7);
    }
    | EIGHTH {
        $$ = MakeOwned<IntLiteral>(8);
    }
    | NINTH {
        $$ = MakeOwned<IntLiteral>(9);
    }
    | TENTH {
        $$ = MakeOwned<IntLiteral>(10);
    }
;

constant
    : TRUE {
        $$ = MakeOwned<StringLiteral>("true");
    }
    | FALSE {
        $$ = MakeOwned<StringLiteral>("false");
    }
    | EMPTY {
        $$ = MakeOwned<StringLiteral>("");
    }
    | RETURN {
        $$ = MakeOwned<StringLiteral>("\n");
    }
    | TAB {
        $$ = MakeOwned<StringLiteral>("\t");
    }
    | SPACE {
        $$ = MakeOwned<StringLiteral>(" ");
    }
    | QUOTE {
        $$ = MakeOwned<StringLiteral>("\"");
    }
    | ZERO {
        $$ = MakeOwned<IntLiteral>(0);
    }
    | ONE {
        $$ = MakeOwned<IntLiteral>(1);
    }
    | TWO {
        $$ = MakeOwned<IntLiteral>(2);
    }
    | THREE {
        $$ = MakeOwned<IntLiteral>(3);
    }
    | FOUR {
        $$ = MakeOwned<IntLiteral>(4);
    }
    | FIVE {
        $$ = MakeOwned<IntLiteral>(5);
    }
    | SIX {
        $$ = MakeOwned<IntLiteral>(6);
    }
    | SEVEN {
        $$ = MakeOwned<IntLiteral>(7);
    }
    | EIGHT {
        $$ = MakeOwned<IntLiteral>(8);
    }
    | NINE {
        $$ = MakeOwned<IntLiteral>(9);
    }
    | TEN {
        $$ = MakeOwned<IntLiteral>(10);
    }
    | PI {
        $$ = MakeOwned<IntLiteral>(M_PI);
    }
;

maybeThe
    : /* empty */
    | THE
;

%%

void yy::parser::error(const yy::parser::location_type &location, const std::string &msg) {
    ctx.error(location.first, msg);
}
