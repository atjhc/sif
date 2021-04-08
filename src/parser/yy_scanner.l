/*
  Copyright (c) 2021 James Callender

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

%{

#include <string>
#include <iostream>

#include "ast/Expression.h"
#include "ast/Chunk.h"
#include "parser/Parser.h"
#include "utilities/strings.h"

using namespace chatter;
using namespace chatter::ast;

#include "yyParser.h"

#define LEX_RETURN(NAME) \
    { \
        return yy::parser::token::NAME; \
    }

#define YY_USER_ACTION \
    yylloc->first = yylloc->last; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            yylloc->last.lineNumber++; \
            yylloc->last.position = 0; \
        } \
        else { \
            yylloc->last.position++; \
        } \
    }

%}

/* Generate line number information. */
%option yylineno 

/* Make the parser reentrant. */
%option reentrant

/* Not using yywrap. */
%option noyywrap

/* Not currently using unput. */
%option nounput

/* Not currently using input. */
%option noinput

/* All Chatter keywords are case insensitive. */
%option case-insensitive

%%

%{
    if (context.selectingMode) {
        context.selectingMode = false;
        switch (context.parsingMode) {
        case ParserContext::Script:
            return yy::parser::token::START_SCRIPT;
        case ParserContext::Statements:
            return yy::parser::token::START_STATEMENT;
        case ParserContext::Expression:
            return yy::parser::token::START_EXPRESSION;
        }
    }
%}

on                          LEX_RETURN(ON);
end                         LEX_RETURN(END);
function                    LEX_RETURN(FUNCTION);
exit                        LEX_RETURN(EXIT);
repeat                      LEX_RETURN(REPEAT);
pass                        LEX_RETURN(PASS);
if                          LEX_RETURN(IF);
then                        LEX_RETURN(THEN);
else                        LEX_RETURN(ELSE);
of                          LEX_RETURN(OF);
put                         LEX_RETURN(PUT);
get                         LEX_RETURN(GET);
ask                         LEX_RETURN(ASK);
into                        LEX_RETURN(INTO);
after                       LEX_RETURN(AFTER);
before                      LEX_RETURN(BEFORE);
while                       LEX_RETURN(WHILE);
global                      LEX_RETURN(GLOBAL);
return                      LEX_RETURN(RETURN);
to                          LEX_RETURN(TO);
the                         LEX_RETURN(THE);
not                         LEX_RETURN(NOT);
and                         LEX_RETURN(AND);
or                          LEX_RETURN(OR);
div                         LEX_RETURN(DIV_TRUNC);
mod                         LEX_RETURN(MOD);
contains                    LEX_RETURN(CONTAINS);
forever                     LEX_RETURN(FOREVER);
times                       LEX_RETURN(TIMES);
until                       LEX_RETURN(UNTIL);
with                        LEX_RETURN(WITH);
down                        LEX_RETURN(DOWN);
is                          LEX_RETURN(IS);
a|an                        LEX_RETURN(AN);
from                        LEX_RETURN(FROM);
by                          LEX_RETURN(BY);
add                         LEX_RETURN(ADD);
subtract                    LEX_RETURN(SUBTRACT);
multiply                    LEX_RETURN(MULTIPLY);
divide                      LEX_RETURN(DIVIDE);
as                          LEX_RETURN(AS);
do                          LEX_RETURN(DO);
next                        LEX_RETURN(NEXT);
program                     LEX_RETURN(PROGRAM);
for                         LEX_RETURN(FOR);
no                          LEX_RETURN(NO);
in                          LEX_RETURN(IN);
within                      LEX_RETURN(WITHIN);

first                       LEX_RETURN(FIRST);
second                      LEX_RETURN(SECOND);
third                       LEX_RETURN(THIRD);
fourth                      LEX_RETURN(FOURTH);
fifth                       LEX_RETURN(FIFTH);
sixth                       LEX_RETURN(SIXTH);
seventh                     LEX_RETURN(SEVENTH);
eighth                      LEX_RETURN(EIGHTH);
ninth                       LEX_RETURN(NINTH);
tenth                       LEX_RETURN(TENTH);
last                        LEX_RETURN(LAST);
(mid)|(middle)              LEX_RETURN(MIDDLE);
any                         LEX_RETURN(ANY);

(char)|(character)          LEX_RETURN(CHAR);
word                        LEX_RETURN(WORD);
line                        LEX_RETURN(LINE);
item                        LEX_RETURN(ITEM);

true                        LEX_RETURN(TRUE);
false                       LEX_RETURN(FALSE);
empty                       LEX_RETURN(EMPTY);
zero                        LEX_RETURN(ZERO);
one                         LEX_RETURN(ONE);
two                         LEX_RETURN(TWO);
three                       LEX_RETURN(THREE);
four                        LEX_RETURN(FOUR);
five                        LEX_RETURN(FIVE);
six                         LEX_RETURN(SIX);
seven                       LEX_RETURN(SEVEN);
eigth                       LEX_RETURN(EIGHT);
nine                        LEX_RETURN(NINE);
ten                         LEX_RETURN(TEN);
tab                         LEX_RETURN(TAB);
space                       LEX_RETURN(SPACE);
quote                       LEX_RETURN(QUOTE);
pi                          LEX_RETURN(PI);

"("                         LEX_RETURN(LPAREN);
")"                         LEX_RETURN(RPAREN);
"&&"                        LEX_RETURN(CONCAT_SPACE);
"&"                         LEX_RETURN(CONCAT);
","                         LEX_RETURN(COMMA);
"+"                         LEX_RETURN(PLUS);
"-"                         LEX_RETURN(MINUS);
"*"                         LEX_RETURN(MULT);
"/"                         LEX_RETURN(DIV);
"<"                         LEX_RETURN(LT);
">"                         LEX_RETURN(GT);
"="                         LEX_RETURN(EQ);
"^"                         LEX_RETURN(CARROT);
"<="|"≤"                    LEX_RETURN(LTE);
">="|"≥"                    LEX_RETURN(GTE);
"≠"|"<>"                    LEX_RETURN(NEQ);

([0-9][0-9]*) {
    auto expression = new IntLiteral(atoi(yytext));
    yylval->emplace<Expression*>(expression);
    LEX_RETURN(INT_LITERAL); 
}

([0-9][0-9]*)(\.[0-9][0-9]*)? { 
    auto expression = new FloatLiteral(atof(yytext));
    yylval->emplace<Owned<Expression>>(expression);
    LEX_RETURN(FLOAT_LITERAL); 
}

\"([^\\\"]|\\.)*\" {
    auto value = string_from_escaped_string(std::string(yytext));
    auto expression = new StringLiteral(value.substr(1, value.length() - 2));
    yylval->emplace<Owned<Expression>>(expression);
    LEX_RETURN(STRING_LITERAL);
}

[a-zA-Z_][a-zA-Z0-9_]* { 
    auto expression = new Identifier(std::string(yytext));
    yylval->emplace<Owned<Expression>>(expression);
    LEX_RETURN(IDENTIFIER); 
}

\¬.*\n {
    /* ignore */ ;
}

\\.*\n {
    /* ignore */ ;
}

\n+ {
    LEX_RETURN(EOL);
}

"--".*\n? {
    /* ignore */ ;
}

"#".*\n? {
    /* ignore */ ;
}

[ \t]* {
    /* ignore */ ;
}

. {
    /* ignore */
}

%%
