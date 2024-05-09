#include "tree.h"

// memory leak???

// copy string
char* my_strdup(const char* s) {
    if (s == NULL) return NULL;
    char* copy = malloc(strlen(s) + 1);
    if (copy != NULL) strcpy(copy, s);
    return copy;
}

// insert new node
TreeNode* createNode(char* name, char* value, int lineNo) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    node->name = my_strdup(name);
    node->value = value ? my_strdup(value) : NULL;
    node->lineNo = lineNo;
    node->children = NULL;
    node->childrenCount = 0;
    return node;
}

// add node
void addNode(TreeNode* parent, int childrenCount, ...) {
    va_list list;
    va_start(list, childrenCount);
    parent->childrenCount = childrenCount;

    // reallocate array
    parent->children = realloc(parent->children, sizeof(TreeNode*)*parent->childrenCount);

    for(int i = 0; i < childrenCount; i++) {
        TreeNode* child = va_arg(list, TreeNode*);
        parent->children[i] = child; // add new child
    }

    //parent->lineNo = parent->children[0]->lineNo;

    va_end(list);
}

// print tree
void printTree(TreeNode* root, int level){
    if (!root) return;

    for(int i=0; i<level; i++){
        printf("  ");
    }
    
    if (root->childrenCount != 0){
        printf("%s (%d)\n", root->name, root->lineNo);
    }
    else{
        if (strcmp(root->name, "ID") == 0){
            printf("%s: %s\n", root->name, root->value);
        }
        else if (strcmp(root->name, "TYPE") == 0){
            printf("%s: %s\n", root->name, root->value);
        }
        else if(strcmp(root->name, "INT") == 0){
            printf("%s: %d\n", root->name, atoi(root->value));
        }
        else if(strcmp(root->name, "INT8") == 0){
            long octalValue = strtol(root->value, NULL, 8);
            printf("INT: %ld\n", octalValue);
        }
        else if(strcmp(root->name, "INT16") == 0){
            long hexValue = strtol(root->value, NULL, 16);
            printf("INT: %ld\n", hexValue);
        }
        else if(strcmp(root->name, "FLOAT") == 0){
            printf("%s: %f\n", root->name, atof(root->value));
        }
        else {
            printf("%s\n", root->name);
        }
    }

    for (int i = 0; i < root->childrenCount; i++) {
        printTree(root->children[i], level + 1);
    }
}