%{

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

#include <string>
#include <iostream>

#include "ast/ast.h"
using namespace hypertalk::ast;

#include "Parser.h"

#define YY_DECL int yylex(YYSTYPE *yylval_param, void *yyscanner, ParserContext &context)

#define LEX_RETURN(token) \
	{ \
		context.currentLocation = context.lookAheadLocation; \
		context.lookAheadLocation.lineNumber = yylineno; \
		return token; \
	}

#define YY_USER_ACTION \
	{ \
		context.lookAheadLocation.position += strlen(yytext); \
	}

%}

%option reentrant bison-bridge
%option noyywrap yylineno

%%

[oO][nN] 					LEX_RETURN(ON);
[eE][nN][dD]				LEX_RETURN(END);
[fF][uU][nN][cC][tT][iI][oO][nN] {
	LEX_RETURN(FUNCTION);
}
[eE][xX][iI][tT]			LEX_RETURN(EXIT);
[rR][eE][pP][eE][aA][tT]	LEX_RETURN(REPEAT);
[pP][aA][sS][sS]			LEX_RETURN(PASS);
[iI][fF] 					LEX_RETURN(IF);
[tT][hH][eE][nN]			LEX_RETURN(THEN);
[eE][lL][sS][eE]			LEX_RETURN(ELSE);
[oO][fF] 					LEX_RETURN(OF);
[pP][uU][tT]				LEX_RETURN(PUT);
[gG][eE][tT] 				LEX_RETURN(GET);
[iI][nN][tT][oO]			LEX_RETURN(INTO);
[wW][hH][iI][lL][eE] 		LEX_RETURN(WHILE);
[gG][lL][oO][bB][aA][lL]	LEX_RETURN(GLOBAL);
[rR][eE][tT][uU][rR][nN]	LEX_RETURN(RETURN);
[tT][oO]					LEX_RETURN(TO);
[tT][hH][eE]				LEX_RETURN(THE);
[nN][oO][tT] 				LEX_RETURN(NOT);
[aA][nN][dD] 				LEX_RETURN(AND);
[oO][rR] 					LEX_RETURN(OR);
[dD][iI][vV]				LEX_RETURN(DIV_TRUNC);
[mM][oO][dD]				LEX_RETURN(MOD);
[cC][oO][nN][tT][aA][iI][nN][sS] {
	LEX_RETURN(CONTAINS);
}

do							LEX_RETURN(DO);
next						LEX_RETURN(NEXT);
send						LEX_RETURN(SEND);
window						LEX_RETURN(WINDOW);
program						LEX_RETURN(PROGRAM);
forever						LEX_RETURN(FOREVER);
with						LEX_RETURN(WITH);
until 						LEX_RETURN(UNTIL);
for 						LEX_RETURN(FOR);
down 						LEX_RETURN(DOWN);
times						LEX_RETURN(TIMES);
an 							LEX_RETURN(AN);
no 							LEX_RETURN(NO);
in 							LEX_RETURN(IN);
within 						LEX_RETURN(WITHIN);
empty						LEX_RETURN(EMPTY);
formFeed 					LEX_RETURN(FORM_FEED);
lineFeed 					LEX_RETURN(LINE_FEED);
tab							LEX_RETURN(TAB);
up 							LEX_RETURN(UP);

"("							LEX_RETURN(LPAREN);
")"							LEX_RETURN(RPAREN);
"&&"						LEX_RETURN(CONCAT_SPACE);
"&"							LEX_RETURN(CONCAT);
","							LEX_RETURN(COMMA);
"+"							LEX_RETURN(PLUS);
"-"							LEX_RETURN(MINUS);
"*"							LEX_RETURN(MULT);
"/"							LEX_RETURN(DIV);
"<"							LEX_RETURN(LT);
">"							LEX_RETURN(GT);
"<="|"≤"					LEX_RETURN(LTE);
">="|"≥"					LEX_RETURN(GTE);
"="|[iI][sS]				LEX_RETURN(IS);
"≠"|"<>"					LEX_RETURN(NEQ);



(([1-9][0-9]*)|0) {
	yylval->expression = new IntLiteral(atoi(yytext));
	LEX_RETURN(INT_LITERAL); 
};

(([1-9][0-9]*)|0)(\.[0-9][0-9]*)? { 
	yylval->expression = new FloatLiteral(atof(yytext));
	LEX_RETURN(FLOAT_LITERAL); 
};

\"([^\\\"]|\\.)*\" {
	yylval->expression = new StringLiteral(std::string(yytext)); 
	LEX_RETURN(STRING_LITERAL);
};

[a-zA-Z_][a-zA-Z0-9_]* { 
	yylval->identifier = new Identifier(std::string(yytext)); 
	LEX_RETURN(IDENTIFIER); 
}

"--".*\n {
	context.lookAheadLocation.position = 1;
	LEX_RETURN(EOL);
}

\¬\n {
	context.lookAheadLocation.position = 1;
}

\n+ {
	context.lookAheadLocation.position = 1;
	LEX_RETURN(EOL);
}

[ \t]* {
	;
}

%%
