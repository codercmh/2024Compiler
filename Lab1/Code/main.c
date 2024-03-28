#include <stdio.h>
#include "syntax.tab.h"
#include "tree.h"

int error_num=0;
TreeNode* root;
extern int yylineno;
extern void yyrestart(FILE*);

int main(int argc, char** argv)
{
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if (error_num==0){
        printTree(root, 0);
    }
    return 0;
}