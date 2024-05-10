#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

#define HASH_SIZE 0x3fff
#define TYPE_INT 1
#define TYPE_FLOAT 2

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;

struct Type_{
    enum { BASIC, ARRAY, STRUCTURE, STRUCT_DEF, FUNC } kind;
    union{
        // 基本类型
        int basic;
        // 数组类型信息包括元素类型与数组大小构成
        struct { Type elem; int size; } array;
        // 结构体类型信息是一个链表
        FieldList structure;
        // 函数类型信息包括函数返回值类型，参数个数，参数类型等
        struct {
            FieldList params;
            int paramNum;
            Type returnType;
        }func;
    } u;
};

struct FieldList_{
    char* name; // 域的名字
    Type type; // 域的类型
    FieldList tail; // 下一个域
};

//define symbol table entry
typedef struct SymbolTableEntry {
    char* name;
    Type type;
    struct SymbolTableEntry* next;
    int isArg;   //是否为函数形参 lab3
} SymbolTableEntry;


//hash table
void initHashTable();
unsigned int hash_pjw(char* name);
void insert(SymbolTableEntry* entry);
SymbolTableEntry* find(char* name, int type);

int typeMatch(Type typea, Type typeb);
char* my_strdup(const char* s);
FieldList createFieldFromEntry(SymbolTableEntry* entry);

//semantic analysis in tree
void semantic_analysis(TreeNode* root);
void ExtDefList(TreeNode* node);
void ExtDef(TreeNode* node);
void ExtDecList(TreeNode* node, Type type);
Type Specifier(TreeNode* node);
Type StructSpecifier(TreeNode* node);
SymbolTableEntry* VarDec(TreeNode* node, Type type);
void FunDec(TreeNode* node, Type type);
void CompSt(TreeNode* node, Type type);
void Stmt(TreeNode* node, Type type);
void DefList(TreeNode* node);
void Def(TreeNode* node);
void DecList(TreeNode* node, Type type);
void Dec(TreeNode* node, Type type);
Type Exp(TreeNode* node);

#endif