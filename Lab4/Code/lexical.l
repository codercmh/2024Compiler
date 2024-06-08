%{
    #include <stdio.h>
    #include "syntax.tab.h"

    extern int error_num;
    int yycolumn =1;

    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
%}

%option yylineno

ws [ \t]
digit [0-9]
letter [_a-zA-Z]

INT 0|([1-9]{digit}*)
INT8 0[0-7]+
INT16 (0x|0X)[0-9A-Fa-f]+
FLOAT {digit}+\.{digit}+
ID {letter}({letter}|{digit})*
SEMI ";"
COMMA ","
ASSIGNOP "="
RELOP ">"|"<"|">="|"<="|"=="|"!="
PLUS "+"
MINUS "-"
STAR "*"
DIV "/"
AND "&&"
OR "||"
DOT "."
NOT "!"
TYPE "int"|"float"
LP "("
RP ")"
LB "["
RB "]"
LC "{"
RC "}"
STRUCT "struct"
RETURN "return"
IF "if"
ELSE "else"
WHILE "while"

%%
{ws}+ {;}
[\n\r]+ {yycolumn = 1;}
{TYPE} {yylval.node=createNode("TYPE", yytext, yylineno);return TYPE;}
{STRUCT} {yylval.node=createNode("STRUCT", yytext, yylineno);return STRUCT;}
{RETURN} {yylval.node=createNode("RETURN", yytext, yylineno);return RETURN;}
{IF} {yylval.node=createNode("IF", yytext, yylineno);return IF;}
{ELSE} {yylval.node=createNode("ELSE", yytext, yylineno);return ELSE;}
{WHILE} {yylval.node=createNode("WHILE", yytext, yylineno);return WHILE;}
{SEMI} {yylval.node=createNode("SEMI", yytext, yylineno);return SEMI;}
{COMMA} {yylval.node=createNode("COMMA", yytext, yylineno);return COMMA;}
{ASSIGNOP} {yylval.node=createNode("ASSIGNOP", yytext, yylineno);return ASSIGNOP;}
{RELOP} {yylval.node=createNode("RELOP", yytext, yylineno);return RELOP;}
{PLUS} {yylval.node=createNode("PLUS", yytext, yylineno);return PLUS;}
{MINUS} {yylval.node=createNode("MINUS", yytext, yylineno);return MINUS;}
{STAR} {yylval.node=createNode("STAR", yytext, yylineno);return STAR;}
{DIV} {yylval.node=createNode("DIV", yytext, yylineno);return DIV;}
{AND} {yylval.node=createNode("AND", yytext, yylineno);return AND;}
{OR} {yylval.node=createNode("OR", yytext, yylineno);return OR;}
{DOT} {yylval.node=createNode("DOT", yytext, yylineno);return DOT;}
{NOT} {yylval.node=createNode("NOT", yytext, yylineno);return NOT;}
{LP} {yylval.node=createNode("LP", yytext, yylineno);return LP;}
{RP} {yylval.node=createNode("RP", yytext, yylineno);return RP;}
{LB} {yylval.node=createNode("LB", yytext, yylineno);return LB;}
{RB} {yylval.node=createNode("RB", yytext, yylineno);return RB;}
{LC} {yylval.node=createNode("LC", yytext, yylineno);return LC;}
{RC} {yylval.node=createNode("RC", yytext, yylineno);return RC;}
{ID} {yylval.node=createNode("ID", yytext, yylineno);return ID;}
{INT} {yylval.node=createNode("INT", yytext, yylineno);return INT;}
{INT8} {yylval.node=createNode("INT8", yytext, yylineno);return INT;}
{INT16} {yylval.node=createNode("INT16", yytext, yylineno);return INT;}
{FLOAT} {yylval.node=createNode("FLOAT", yytext, yylineno);return FLOAT;}
0[xX][0-9a-zA-Z]* {printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext);error_num++;}
0[0-9a-zA-Z]+ {printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext);error_num++;}
. {printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);error_num++;}
%%
/*
int main(int argc, char** argv) {
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    while (yylex() != 0);
    return 0;
}
*/