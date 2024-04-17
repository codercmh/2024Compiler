#include <stdio.h>
#include "syntax.tab.h"
#include "tree.h"
#include "semantic.h"

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
    int t=yyparse();
    //printf("%d/n", t);
    //printf("%d/n", error_num);
    //if (error_num==0 && t==0){
    //    printTree(root, 0);
    //}
    semantic_analysis(root);
    return 0;
}