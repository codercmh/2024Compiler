%{
    #include <stdio.h>
%}

%option yylineno

ws [ \t\n\r]
digit [0-9]
letter [_a-zA-Z]

INT 0|([1-9]{digit}*)|(0[0-7]+)|((0x|0X)[0-9A-Fa-f]+)
FLOAT ({digit}+\.{digit}+)|({digit}+\.{digit}*[Ee][+-]?{digit}+)|({digit}*\.{digit}+[Ee][+-]?{digit}+)
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
{ws}+ {printf("white space\n");}
{TYPE} {printf("TYPE\n");}
{STRUCT} {printf("STRUCT\n");}
{RETURN} {printf("RETURN\n");}
{IF} {printf("IF\n");}
{ELSE} {printf("ELSE\n");}
{WHILE} {printf("WHILE\n");}
{INT} {printf("INT\n");}
{FLOAT} {printf("FLOAT\n");}
{ID} {printf("ID\n");}
{SEMI} {printf("SEMI\n");}
{COMMA} {printf("COMMA\n");}
{ASSIGNOP} {printf("ASSIGNOP\n");}
{RELOP} {printf("RELOP\n");}
{PLUS} {printf("PLUS\n");}
{MINUS} {printf("MINUS\n");}
{STAR} {printf("STAR\n");}
{DIV} {printf("DIV\n");}
{AND} {printf("AND\n");}
{OR} {printf("OR\n");}
{DOT} {printf("DOT\n");}
{NOT} {printf("NOT\n");}
{LP} {printf("LP\n");}
{RP} {printf("RP\n");}
{LB} {printf("LB\n");}
{RB} {printf("RB\n");}
{LC} {printf("LC\n");}
{RC} {printf("RC\n");}
. {printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);}
%%
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