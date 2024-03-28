%{
    #include <stdio.h>
    #include "tree.h"
    #include "lex.yy.c"

    extern int yylineno;
    extern TreeNode* root;
    extern int error_num;

    void yyerror(char const *msg){
        if (msg != "syntax error"){
            printf("Error type B at Line %d: %s\n", yylineno, msg);
        }
}
%}

%locations

%union{
    struct TreeNode* node;
}

//tokens
%token <node> INT
%token <node> FLOAT
%token <node> ID
%token <node> TYPE
%token <node> COMMA
%token <node> DOT
%token <node> SEMI
%token <node> RELOP
%token <node> ASSIGNOP
%token <node> PLUS MINUS STAR DIV
%token <node> AND OR NOT 
%token <node> LP RP LB RB LC RC
%token <node> IF
%token <node> ELSE
%token <node> WHILE
%token <node> STRUCT
%token <node> RETURN

//non-terminal
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def DecList Dec
%type <node> Exp Args

//precedence & associativity
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left DOT
%left LB RB
%left LP RP
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
//High-level Definitions
Program : ExtDefList    {$$=createNode("Pragram", NULL, @$.first_line);addNode($$, 1, $1);root=$$;}
    ;
ExtDefList : ExtDef ExtDefList    {$$=createNode("ExtDefList", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    |    {$$=NULL;}
    ;
ExtDef : Specifier ExtDecList SEMI    {$$=createNode("ExtDef", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Specifier SEMI    {$$=createNode("ExtDef", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    | Specifier FunDec CompSt    {$$=createNode("ExtDef", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | error SEMI    {error_num++;yyerror("Syntax error in high-level definitions");}
    ;
ExtDecList : VarDec    {$$=createNode("ExtDecList", NULL, @$.first_line);addNode($$, 1, $1);}
    | VarDec COMMA ExtDecList    {$$=createNode("ExtDecList", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    ;

//Specifiers
Specifier : TYPE    {$$=createNode("Specifier", NULL, @$.first_line);addNode($$, 1, $1);}
    | StructSpecifier    {$$=createNode("Specifier", NULL, @$.first_line);addNode($$, 1, $1);}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC    {$$=createNode("StructSpecifier", NULL, @$.first_line);addNode($$, 5, $1, $2, $3, $4, $5);}
    | STRUCT Tag    {$$=createNode("StructSpecifier", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    ;
OptTag : ID    {$$=createNode("OptTag", NULL, @$.first_line);addNode($$, 1, $1);}
    |    {$$=NULL;}
    ;
Tag : ID    {$$=createNode("Tag", NULL, @$.first_line);addNode($$, 1, $1);}
    ;

//Declarators
VarDec : ID    {$$=createNode("VarDec", NULL, @$.first_line);addNode($$, 1, $1);}
    | VarDec LB INT RB    {$$=createNode("VarDec", NULL, @$.first_line);addNode($$, 4, $1, $2, $3, $4);}
    | error RB    {error_num++;yyerror("Syntax error in declarators");}
    | VarDec LB error RB    {error_num++;yyerror("Missing \"]\".");}
    ;
FunDec : ID LP VarList RP    {$$=createNode("FunDec", NULL, @$.first_line);addNode($$, 4, $1, $2, $3, $4);}
    | ID LP RP    {$$=createNode("FunDec", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | error RP    {error_num++;yyerror("Syntax error in declarators");}
    ;
VarList : ParamDec COMMA VarList    {$$=createNode("VarList", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | ParamDec    {$$=createNode("VarList", NULL, @$.first_line);addNode($$, 1, $1);}
    ;
ParamDec : Specifier VarDec    {$$=createNode("VarList", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    ;

//Statements
CompSt : LC DefList StmtList RC    {$$=createNode("CompSt", NULL, @$.first_line);addNode($$, 4, $1, $2, $3, $4);}
    | error RC    {error_num++;yyerror("Syntax error in statements");}
    ;
StmtList : Stmt StmtList    {$$=createNode("StmtList", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    |    {$$=NULL;}
    ;
Stmt : Exp SEMI    {$$=createNode("Stmt", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    | CompSt        {$$=createNode("Stmt", NULL, @$.first_line);addNode($$, 1, $1);}
    | RETURN Exp SEMI    {$$=createNode("Stmt", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE    {$$=createNode("Stmt", NULL, @$.first_line);addNode($$, 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt    {$$=createNode("Stmt", NULL, @$.first_line);addNode($$, 7, $1, $2, $3, $4, $5, $6, $7);}
    | IF LP Exp RP error ELSE Stmt    {error_num++;yyerror("Missing \";\".");}
    | WHILE LP Exp RP Stmt    {$$=createNode("Stmt", NULL, @$.first_line);addNode($$, 5, $1, $2, $3, $4, $5);}
    | error SEMI    {error_num++;yyerror("Syntax error in statements");}
    ;

//Local Definitions
DefList : Def DefList    {$$=createNode("DefList", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    |    {$$=NULL;}
    ;
Def : Specifier DecList SEMI    {$$=createNode("Def", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    ;
DecList : Dec    {$$=createNode("DecList", NULL, @$.first_line);addNode($$, 1, $1);}
    | Dec COMMA DecList    {$$=createNode("DecList", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    ;
Dec : VarDec    {$$=createNode("Dec", NULL, @$.first_line);addNode($$, 1, $1);}
    | VarDec ASSIGNOP Exp    {$$=createNode("Dec", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    ;

//Expressions
Exp : Exp ASSIGNOP Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp AND Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp OR Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp RELOP Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp PLUS Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp MINUS Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp STAR Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp DIV Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | LP Exp RP    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | MINUS Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    | NOT Exp    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 2, $1, $2);}
    | ID LP Args RP    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 4, $1, $2, $3, $4);}
    | ID LP RP    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp LB Exp RB    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 4, $1, $2, $3, $4);}
    | Exp LB error RB    {error_num++;yyerror("Missing \"]\".");}
    | Exp DOT ID    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | ID    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 1, $1);}
    | INT    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 1, $1);}
    | FLOAT    {$$=createNode("Exp", NULL, @$.first_line);addNode($$, 1, $1);}
    | error RP    {error_num++;yyerror("Syntax error in expressions");}
    ;
Args : Exp COMMA Args    {$$=createNode("Args", NULL, @$.first_line);addNode($$, 3, $1, $2, $3);}
    | Exp    {$$=createNode("Args", NULL, @$.first_line);addNode($$, 1, $1);}
    ;
%%