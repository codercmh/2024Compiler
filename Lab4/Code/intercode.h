#ifndef INTERCODE_H
#define INTERCODE_H

#include "tree.h"
#include "semantic.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//操作数
typedef struct Operand_* Operand;
struct Operand_ {
    enum {
        OP_VARIABLE,
        OP_CONSTANT,
        OP_ADDRESS,
        OP_LABEL,
        OP_FUNCTION,
        OP_RELOP,
    } kind;

    union {
        int value;
        char* name;
    } u;

};

//用双向链表保存中间代码
typedef struct InterCode_* InterCode;
struct InterCode_ {
    enum {
        IR_LABEL,
        IR_FUNCTION,
        IR_ASSIGN,
        IR_ADD,
        IR_SUB,
        IR_MUL,
        IR_DIV,
        IR_GET_ADDR,
        IR_READ_ADDR,
        IR_WRITE_ADDR,
        IR_GOTO,
        IR_IF_GOTO,
        IR_RETURN,
        IR_DEC,
        IR_ARG,
        IR_CALL,
        IR_PARAM,
        IR_READ,
        IR_WRITE,
    } kind;

    union {
        struct {
            Operand op;
        } oneOp;
        struct {
            Operand right, left;
        } assign;
        struct {
            Operand result, op1, op2;
        } binOp;
        struct {
            Operand x, relop, y, z;
        } ifGoto;
        struct {
            Operand op;
            int size;
        } dec;
    } u;

    //上一条/下一条中间代码
    InterCode prev, next;
};

typedef struct InterCodeList_* InterCodeList;
struct InterCodeList_ {
    InterCode head;
    InterCode cur;
    //char* lastArrayName;  // 针对结构体数组，因为需要数组名查表
    int tempVarNum;
    int labelNum;
};

typedef struct arg_* Arg;
struct arg_ {
    Operand op;
    Arg next;
};

typedef struct argList_* ArgList;
struct argList_ {
    Arg head;
    Arg cur;
};

extern InterCodeList IR;

//intercode
void initIR();
InterCode newInterCode(int kind, ...);
void insertInterCode(InterCode newCode);
void genInterCode(int kind, ...);
Operand newTemp();
Operand newLabel();
int getSize(Type type);
Arg newArg(Operand op);
ArgList newArgList();
void addArg(ArgList argList, Arg arg);
void printOp(FILE* fp, Operand op);
void printInterCode(FILE* fp);


//generate intercode in tree
void get_intercode(TreeNode *root);
void translateExtDefList(TreeNode *node);
void translateExtDef(TreeNode *node);
void translateFunDec(TreeNode *node);
void translateCompSt(TreeNode* node);
void translateDefList(TreeNode* node);
void translateDef(TreeNode* node);
void translateDecList(TreeNode* node);
void translateDec(TreeNode* node);
void translateVarDec(TreeNode* node, Operand place);
void translateStmtList(TreeNode* node);
void translateStmt(TreeNode* node);
void translateCond(TreeNode* node, Operand labelTrue, Operand labelFalse);
void translateExp(TreeNode* node, Operand place);
void translateArgs(TreeNode* node, ArgList argList);


#endif