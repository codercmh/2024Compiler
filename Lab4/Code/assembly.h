#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "intercode.h"

#define REG_NUM 32

typedef enum {
    ZERO,
    AT,
    V0,
    V1,
    A0,
    A1,
    A2,
    A3,
    //可以随意使用的
    //begin
    T0,
    T1,
    T2,
    T3,
    T4,
    T5,
    T6,
    T7,
    S0,
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7,
    T8,
    T9,
    //end
    K0,
    K1,
    GP,
    SP,
    FP,
    RA,
} RegNo;

typedef struct Register_* Register;
typedef struct Register_ {
    int isFree;
    char* name;
}Register_;

typedef struct Varible_* Varible;
typedef struct Varible_ {
    int regNo;
    Operand op;
    Varible next;
}Varible_;

typedef struct Registers_* Registers;
typedef struct Registers_ {
    Register regList[REG_NUM];
    int lastChangedNo;
}Registers_;

typedef struct VarList_* VarList;
typedef struct VarList_ {
    Varible head;
    Varible cur;
}VarList_;

typedef struct VarTable_* VarTable;
typedef struct VarTable_ {
    VarList varlistReg; // 寄存器中的变量表
    VarList varlistMem; // 内存中的变量表
    int inFunc;
    char* curFuncName;
}VarTable_;


extern Registers registers;
extern VarTable varTable;


Registers initRegisters();
Register newRegister(const char* regName);
void resetRegisters(Registers registers);

VarTable initVarTable();
VarList newVarList();
void clearVarList(VarList varList);
Varible newVarible(int regNo, Operand op);
void addVarible(VarList varList, int regNo, Operand op);
void delVarible(VarList varList, Varible var);

int allocReg(FILE* fp, VarTable varTable, Registers registers, Operand op);
int allocate(Registers registers, VarTable varTable, Operand op);

void get_assemblycode(FILE* fp);
void initCode(FILE* fp);
void interToAssembly(FILE* fp, InterCode intercode);
void pusha(FILE* fp);
void popa(FILE* fp);



#endif