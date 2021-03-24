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
#include "parser/Parser.h"

using namespace chatter;
using namespace chatter::ast;

#include "yyParser.h"

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
[aA][sS][kK] 				LEX_RETURN(ASK);
[iI][nN][tT][oO]			LEX_RETURN(INTO);
[aA][fF][tT][eE][rR]		LEX_RETURN(AFTER);
[bB][eE][fF][oO][rR][eE]	LEX_RETURN(BEFORE);
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
[fF][oO][rR][eE][vV][eE][rR] {
	LEX_RETURN(FOREVER);
}
[tT][iI][mM][eE][sS]		LEX_RETURN(TIMES);
[uU][nN][tT][iI][lL]		LEX_RETURN(UNTIL);
[wW][iI][tT][hH]			LEX_RETURN(WITH);
[dD][oO][wW][nN]			LEX_RETURN(DOWN);
[iI][sS]					LEX_RETURN(IS);

[fF][iI][rR][sS][tT]		LEX_RETURN(FIRST);
[sS][eE][cC][oO][nN][dD]	LEX_RETURN(SECOND);
[tT][hH][iI][rR][dD]		LEX_RETURN(THIRD);
[fF][oO][uU][rR][tT][hH]	LEX_RETURN(FOURTH);
[fF][iI][fF][tT][hH]		LEX_RETURN(FIFTH);
[sS][iI][xX][tT][hH]		LEX_RETURN(SIXTH);
[sS][eE][vV][eE][nN][tT][hH] LEX_RETURN(SEVENTH);
[eE][iI][gG][hH][tT][hH]	LEX_RETURN(EIGHTH);
[nN][iI][nN][tT][hH]		LEX_RETURN(NINTH);
[tT][eE][nN][tT][hH]		LEX_RETURN(TENTH);
[lL][aA][sS][tT]			LEX_RETURN(LAST);
[mM][iI][dD]				LEX_RETURN(MIDDLE);
[mM][iI][dD][dD][lL][eE]	LEX_RETURN(MIDDLE);
[aA][nN][yY]				LEX_RETURN(ANY);

[cC][hH][aA][rR]			LEX_RETURN(CHAR);
[wW][oO][rR][dD]			LEX_RETURN(WORD);
[lL][iI][nN][eE]			LEX_RETURN(LINE);
[iI][tT][eE][mM]			LEX_RETURN(ITEM);

[tT][rR][uU][eE]			LEX_RETURN(TRUE);
[fF][aA][lL][sS][eE]		LEX_RETURN(FALSE);
[eE][mM][pP][tT][yY]		LEX_RETURN(EMPTY);
[zZ][eE][rR][oO]			LEX_RETURN(ZERO);
[oO][nN][eE]				LEX_RETURN(ONE);
[tT][wW][oO]				LEX_RETURN(TWO);
[tT][hH][rR][eE][eE]		LEX_RETURN(THREE);
[fF][oO][uU][rR]			LEX_RETURN(FOUR);
[fF][iI][vV][eE]			LEX_RETURN(FIVE);
[sS][iI][xX]				LEX_RETURN(SIX);
[sS][eE][vV][eE][nN]		LEX_RETURN(SEVEN);
[eE][iI][gG][hH][tT]		LEX_RETURN(EIGHT);
[nN][iI][nN][eE]			LEX_RETURN(NINE);
[tE][eE][nN]				LEX_RETURN(TEN);
[tT][aA][bB]				LEX_RETURN(TAB);
[sS][pP][aA][cC][eE]		LEX_RETURN(SPACE);
[qQ][uU][oO][tT][eE]		LEX_RETURN(QUOTE);

do							LEX_RETURN(DO);
next						LEX_RETURN(NEXT);
send						LEX_RETURN(SEND);
window						LEX_RETURN(WINDOW);
program						LEX_RETURN(PROGRAM);
for 						LEX_RETURN(FOR);
an 							LEX_RETURN(AN);
no 							LEX_RETURN(NO);
in 							LEX_RETURN(IN);
within 						LEX_RETURN(WITHIN);
formFeed 					LEX_RETURN(FORM_FEED);
lineFeed 					LEX_RETURN(LINE_FEED);
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
"="							LEX_RETURN(EQ);
"<="|"≤"					LEX_RETURN(LTE);
">="|"≥"					LEX_RETURN(GTE);
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
	auto value = std::string(yytext);
	yylval->expression = new StringLiteral(value.substr(1, value.length() - 2)); 
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