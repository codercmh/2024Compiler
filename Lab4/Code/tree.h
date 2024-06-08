#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*
typedef enum {
    INT,
    FLOAT,
    ID,
    TYPE,
    OTHER_TOKEN,
    NOT_TOKEN
} NodeType;
*/

// define tree node
typedef struct TreeNode {
    char* name;
    char* value;
    //NodeType type;
    int lineNo;
    struct TreeNode** children;
    int childrenCount;
} TreeNode;

TreeNode* createNode(char* name, char* value, int lineNo);
void addNode(TreeNode* parent, int childrenCount, ...);
void printTree(TreeNode* root, int level);

#endif
