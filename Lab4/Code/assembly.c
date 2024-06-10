#include "assembly.h"

const char* REG_NAME[REG_NUM] = {
    "$0",  "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2",
    "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5",
    "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};


Registers registers;
VarTable varTable;


Registers initRegisters(){
    Registers p=(Registers)malloc(sizeof(Registers_));
    for(int i=0;i<REG_NUM;i++){
        p->regList[i]=newRegister(REG_NAME[i]);
    }
    p->regList[0]->isFree=0; // $0不允许使用
    p->lastChangedNo=0;
    return p;
}

Register newRegister(const char* regName) {
    Register p = (Register)malloc(sizeof(Register_));
    assert(p != NULL);
    p->isFree = 1;
    p->name = regName;
}

void resetRegisters(Registers registers){
    for(int i=1;i<REG_NUM;i++){
        registers->regList[i]->isFree=1;
    }
}

VarTable initVarTable(){
    VarTable p=(VarTable)malloc(sizeof(VarTable_));
    p->varlistReg=newVarList();
    p->varlistMem=newVarList();
    p->inFunc=0;
    p->curFuncName=NULL;
    return p;
}

VarList newVarList() {
    VarList p = (VarList)malloc(sizeof(VarList_));
    assert(p != NULL);
    p->head = NULL;
    p->cur = NULL;
    return p;
}

void clearVarList(VarList varList){
    if(varList == NULL){
        return;
    }
    Varible head = varList->head;
    while (head) {
        Varible p = head;
        head = head->next;
        free(p);
    }
    varList->head = NULL;
    varList->cur = NULL;
}

Varible newVarible(int regNo, Operand op){
    Varible p=(Varible)malloc(sizeof(Varible_));
    assert(p!=NULL);
    p->regNo=regNo;
    p->op=op;
    p->next=NULL;
    return p;
}

void addVarible(VarList varList, int regNo, Operand op) {
    Varible newVar = newVarible(regNo, op);
    if (varList->head == NULL) {
        varList->head = newVar;
        varList->cur = newVar;
    } else {
        varList->cur->next = newVar;
        varList->cur = newVar;
    }
}

void delVarible(VarList varList, Varible var) {
    if (var == varList->head) {
        varList->head = varList->head->next;
    } else {
        Varible temp = varList->head;
        while (temp) {
            if (temp->next == var) break;
            temp = temp->next;
        }
        if (varList->cur == var) varList->cur = temp;
        temp->next = var->next;
        var->next = NULL;
        free(var);
    }
}

int allocReg(FILE* fp, VarTable varTable, Registers registers, Operand op){
    if (op->kind != OP_CONSTANT) {
        // 若为变量，先看变量表里有没有，有就返回，没有新分配一个
        Varible head = varTable->varlistReg->head;
        while (head) {
            if (head->op->kind != OP_CONSTANT &&
                !strcmp(head->op->u.name, op->u.name))
                return head->regNo;
            head = head->next;
        }
        int regNo = allocate(registers, varTable, op);
        return regNo;
    } else {
        // 立即数则找个寄存器放进去, 如果为0直接返回0号寄存器
        if (op->u.value == 0) return ZERO;
        int regNo = allocate(registers, varTable, op);
        fprintf(fp, "  li %s, %d\n", registers->regList[regNo]->name, op->u.value);
        return regNo;
    }
}

int allocate(Registers registers, VarTable varTable, Operand op){
    // 先看有无空闲，有就直接放
    // printf("allocNewReg\n");
    for (int i = T0; i <= T9; i++) {
        if (registers->regList[i]->isFree) {
            registers->regList[i]->isFree = 0;
            addVarible(varTable->varlistReg, i, op);
            return i;
        }
    }
    // printf("nofree\n");
    // 无空闲了，需要找一个寄存器释放掉
    // 可以先释放掉最近没用过的立即数
    Varible temp = varTable->varlistReg->head;
    while (temp) {
        if (temp->op->kind == OP_CONSTANT &&
            temp->regNo != registers->lastChangedNo) {
            int regNo = temp->regNo;
            registers->lastChangedNo = regNo;
            delVarible(varTable->varlistReg, temp);
            addVarible(varTable->varlistReg, regNo, op);
            return regNo;
        }
        temp = temp->next;
    }

    // 如果没有立即数，就找一个最近没用过的临时变量的释放掉
    // 这里是否一定能找到一个符合条件的临时变量？
    temp = varTable->varlistReg->head;
    while (temp) {
        if (temp->op->kind != OP_CONSTANT) {
            if (temp->op->u.name[0] == 't' &&
                temp->regNo != registers->lastChangedNo) {
                int regNo = temp->regNo;
                registers->lastChangedNo = regNo;
                delVarible(varTable->varlistReg, temp);
                addVarible(varTable->varlistReg, regNo, op);
                return regNo;
            }
        }
        temp = temp->next;
    }
}

void get_assemblycode(FILE* fp){
    registers=initRegisters();
    varTable=initVarTable();
    initCode(fp);

    //逐行翻译中间代码
    InterCode head=IR->head;
    while(head){
        interToAssembly(fp, head);
        head=head->next;
    }
}

void initCode(FILE* fp) {
    fprintf(fp, ".data\n");
    fprintf(fp, "_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(fp, "_ret: .asciiz \"\\n\"\n");
    fprintf(fp, ".globl main\n");

    // 无重复变量名，直接把数组当全局变量声明了
    InterCode head = IR->head;
    while (head) {
        if (head->kind == IR_DEC){
            fprintf(fp, "%s: .word %d\n", head->u.dec.op->u.name, head->u.dec.size);
        }
        head=head->next;
    }

    // read function
    fprintf(fp, ".text\n");
    fprintf(fp, "read:\n");
    fprintf(fp, "  li $v0, 4\n");
    fprintf(fp, "  la $a0, _prompt\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  li $v0, 5\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  jr $ra\n\n");

    // write function
    fprintf(fp, "write:\n");
    fprintf(fp, "  li $v0, 1\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  li $v0, 4\n");
    fprintf(fp, "  la $a0, _ret\n");
    fprintf(fp, "  syscall\n");
    fprintf(fp, "  move $v0, $0\n");
    fprintf(fp, "  jr $ra\n");
}

void interToAssembly(FILE* fp, InterCode intercode){
    
    int kind=intercode->kind;

    if(kind==IR_LABEL){
        fprintf(fp, "%s:\n", intercode->u.oneOp.op->u.name);
    } else if (kind==IR_FUNCTION){
        fprintf(fp, "\n%s:\n", intercode->u.oneOp.op->u.name);

        // 新函数，寄存器重新变为可用，并清空变量表（因为假定没有全局变量）
        resetRegisters(registers);
        clearVarList(varTable->varlistReg);
        clearVarList(varTable->varlistMem);

        // main函数单独处理一下，在main里调用函数不算函数嵌套调用
        if (!strcmp(intercode->u.oneOp.op->u.name, "main")) {
            varTable->inFunc = 0;
            varTable->curFuncName = NULL;
        } else {
            varTable->inFunc = 1;
            varTable->curFuncName = intercode->u.oneOp.op->u.name;

            // 处理形参 IR_PARAM
            SymbolTableEntry* entry = find(intercode->u.oneOp.op->u.name, 1);
            int argc = 0;
            InterCode temp = intercode->next;
            while (temp && temp->kind == IR_PARAM) {
                // 前4个参数存到a0到a3中
                if(argc < 4){
                    addVarible(varTable->varlistReg, A0 + argc,
                               temp->u.oneOp.op);
                }else{
                    // 剩下的要用栈存
                    int regNo = allocReg(fp, varTable, registers,
                                             temp->u.oneOp.op);
                    fprintf(
                        fp, "  lw %s, %d($fp)\n",
                        registers->regList[regNo]->name,
                        (entry->type->u.func.paramNum - 1 - argc) * 4);
                }
                argc++;
                temp = temp->next;
            }
        }
    } else if (kind==IR_GOTO){
        fprintf(fp, "  j %s\n", intercode->u.oneOp.op->u.name);
    } else if (kind==IR_RETURN){
        int RegNo=allocReg(fp, varTable, registers, intercode->u.oneOp.op);
        fprintf(fp, "  move $v0, %s\n", registers->regList[RegNo]->name);
        fprintf(fp, "  jr $ra\n");
    } else if (kind == IR_ARG) {
        // 在call里处理
    } else if (kind == IR_PARAM) {
        // 在function里处理
    } else if (kind == IR_READ) {
        fprintf(fp, "  addi $sp, $sp, -4\n");
        fprintf(fp, "  sw $ra, 0($sp)\n");
        fprintf(fp, "  jal read\n");
        fprintf(fp, "  lw $ra, 0($sp)\n");
        fprintf(fp, "  addi $sp, $sp, 4\n");
        int RegNo = allocReg(fp, varTable, registers, intercode->u.oneOp.op);
        fprintf(fp, "  move %s, $v0\n", registers->regList[RegNo]->name);
    } else if (kind == IR_WRITE) {
        int RegNo = allocReg(fp, varTable, registers, intercode->u.oneOp.op);
        if (varTable->inFunc == 0) {
            fprintf(fp, "  move $a0, %s\n", registers->regList[RegNo]->name);
            fprintf(fp, "  addi $sp, $sp, -4\n");
            fprintf(fp, "  sw $ra, 0($sp)\n");
            fprintf(fp, "  jal write\n");
            fprintf(fp, "  lw $ra, 0($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 4\n");
        } else {
            // 函数嵌套调用，先将a0压栈 调用结束以后需要恢复a0
            fprintf(fp, "  addi $sp, $sp, -8\n");
            fprintf(fp, "  sw $a0, 0($sp)\n");
            fprintf(fp, "  sw $ra, 4($sp)\n");
            fprintf(fp, "  move $a0, %s\n", registers->regList[RegNo]->name);
            fprintf(fp, "  jal write\n");
            fprintf(fp, "  lw $a0, 0($sp)\n");
            fprintf(fp, "  lw $ra, 4($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 8\n");
        }
    } else if (kind == IR_ASSIGN) {
        int leftRegNo =allocReg(fp, varTable, registers, intercode->u.assign.left);
        // 右值为立即数，直接放左值寄存器里
        if (intercode->u.assign.right->kind == OP_CONSTANT) {
            fprintf(fp, "  li %s, %d\n", registers->regList[leftRegNo]->name,
                    intercode->u.assign.right->u.value);
        }
        // 右值为变量，先check再move赋值寄存器
        else {
            int rightRegNo = allocReg(fp, varTable, registers, intercode->u.assign.right);
            fprintf(fp, "  move %s, %s\n", registers->regList[leftRegNo]->name,
                    registers->regList[rightRegNo]->name);
        }
    } else if (kind == IR_GET_ADDR) {
        int leftRegNo = allocReg(fp, varTable, registers, intercode->u.assign.left);
        // int rightRegNo =
        //     allocReg(fp, varTable, registers, intercode->u.assign.right);
        fprintf(fp, "  la %s, %s\n", registers->regList[leftRegNo]->name, intercode->u.assign.right->u.name);
        // fprintf(fp, "  move %s, %s\n", registers->regList[leftRegNo]->name,
        //         registers->regList[rightRegNo]->name);
    } else if (kind == IR_READ_ADDR) {
        int leftRegNo =
            allocReg(fp, varTable, registers, intercode->u.assign.left);
        int rightRegNo =
            allocReg(fp, varTable, registers, intercode->u.assign.right);
        fprintf(fp, "  lw %s, 0(%s)\n", registers->regList[leftRegNo]->name,
                registers->regList[rightRegNo]->name);
    } else if (kind == IR_WRITE_ADDR) {
        int leftRegNo =
            allocReg(fp, varTable, registers, intercode->u.assign.left);
        int rightRegNo =
            allocReg(fp, varTable, registers, intercode->u.assign.right);
        fprintf(fp, "  sw %s, 0(%s)\n", registers->regList[rightRegNo]->name,
                registers->regList[leftRegNo]->name);
    } else if (kind == IR_CALL) {
        SymbolTableEntry* calledFunc =
            find(intercode->u.assign.right->u.name, 1);
        int leftRegNo =
            allocReg(fp, varTable, registers, intercode->u.assign.left);
        // 函数调用前的准备
        fprintf(fp, "  addi $sp, $sp, -4\n");
        fprintf(fp, "  sw $ra, 0($sp)\n");
        pusha(fp);


        // 如果是函数嵌套调用，把前形参存到内存，腾出a0-a3寄存器给新调用使用
        if (varTable->inFunc) {
            fprintf(fp, "  addi $sp, $sp, -%d\n",
                    calledFunc->type->u.func.paramNum * 4);
            SymbolTableEntry* curFunc = find(varTable->curFuncName,1);
            for (int i = 0; i < curFunc->type->u.func.paramNum; i++) {
                if (i > calledFunc->type->u.func.paramNum) break;
                if (i < 4) {
                    fprintf(fp, "  sw %s, %d($sp)\n",
                            registers->regList[A0 + i]->name, i * 4);
                    Varible var = varTable->varlistReg->head;
                    while (var && var->regNo != A0 + i) {
                        var = var->next;
                    }
                    delVarible(varTable->varlistReg, var);
                    addVarible(varTable->varlistMem, -1, var->op);
                    int regNo = allocReg(fp, varTable, registers, var->op);
                    fprintf(fp, "  move %s, %s\n",
                            registers->regList[regNo]->name,
                            registers->regList[A0 + i]->name);
                }
            }
        }

        // 处理实参 IR_ARG
        InterCode arg = intercode->prev;
        int argc = 0;
        while (arg && argc < calledFunc->type->u.func.paramNum) {
            if (arg->kind == IR_ARG) {
                int argRegNo = allocReg(fp, varTable, registers,
                                            arg->u.oneOp.op);
                // 前4个参数直接用寄存器存
                if (argc < 4) {
                    fprintf(fp, "  move %s, %s\n",
                            registers->regList[A0 + argc]->name,
                            registers->regList[argRegNo]->name);
                    argc++;
                }
                // 4个以后的参数压栈
                else {
                    fprintf(fp, "  addi $sp, $sp, -4\n");
                    fprintf(fp, "  sw %s, 0($sp)\n",
                            registers->regList[argRegNo]->name);
                    fprintf(fp, "  move $fp, $sp\n");
                    argc++;
                }
            }
            arg = arg->prev;
        }

        // 函数调用
        fprintf(fp, "  jal %s\n", intercode->u.assign.right->u.name);

        // 调用完后恢复栈指针、形参，然后恢复之前保存入栈的寄存器信息
        if (argc > 4) fprintf(fp, "  addi $sp, $sp, %d\n", 4 * argc);
        if (varTable->inFunc) {
            SymbolTableEntry* curFunc = find(varTable->curFuncName, 1);
            for (int i = 0; i < curFunc->type->u.func.paramNum; i++) {
                if (i > calledFunc->type->u.func.paramNum) break;
                if (i < 4) {
                    fprintf(fp, "  lw %s, %d($sp)\n",
                            registers->regList[A0 + i]->name, i * 4);
                    Varible var = varTable->varlistReg->head;
                    while (var) {
                        if (var->op->kind != OP_CONSTANT &&
                            !strcmp(varTable->varlistMem->head->op->u.name,
                                    var->op->u.name))
                            break;
                        var = var->next;
                    }
                    if (var) {
                        registers->regList[var->regNo]->isFree = 1;
                        var->regNo = A0 + i;
                    } else {
                        addVarible(varTable->varlistReg, A0 + i,
                                   varTable->varlistMem->head->op);
                    }
                    delVarible(varTable->varlistMem,
                               varTable->varlistMem->head);
                }
            }
            fprintf(fp, "  addi $sp, $sp, %d\n",
                    calledFunc->type->u.func.paramNum * 4);
        }
        popa(fp);
        fprintf(fp, "  lw $ra, 0($sp)\n");
        fprintf(fp, "  addi $sp, $sp, 4\n");
        fprintf(fp, "  move %s, $v0\n", registers->regList[leftRegNo]->name);
    } else if (kind == IR_ADD) {
        int resultRegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.result);
        // 常数 常数
        if (intercode->u.binOp.op1->kind == OP_CONSTANT &&
            intercode->u.binOp.op2->kind == OP_CONSTANT) {
            fprintf(fp, "  li %s, %d\n", registers->regList[resultRegNo]->name,
                    intercode->u.binOp.op1->u.value +
                        intercode->u.binOp.op2->u.value);
        }
        // 变量 常数
        else if (intercode->u.binOp.op1->kind != OP_CONSTANT &&
                 intercode->u.binOp.op2->kind == OP_CONSTANT) {
            int op1RegNo =
                allocReg(fp, varTable, registers, intercode->u.binOp.op1);
            fprintf(fp, "  addi %s, %s, %d\n",
                    registers->regList[resultRegNo]->name,
                    registers->regList[op1RegNo]->name,
                    intercode->u.binOp.op2->u.value);
        }
        // 变量 变量
        else {
            int op1RegNo =
                allocReg(fp, varTable, registers, intercode->u.binOp.op1);
            int op2RegNo =
                allocReg(fp, varTable, registers, intercode->u.binOp.op2);
            fprintf(fp, "  add %s, %s, %s\n",
                    registers->regList[resultRegNo]->name,
                    registers->regList[op1RegNo]->name,
                    registers->regList[op2RegNo]->name);
        }
    } else if (kind == IR_SUB) {
        int resultRegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.result);
        // 常数 常数
        if (intercode->u.binOp.op1->kind == OP_CONSTANT &&
            intercode->u.binOp.op2->kind == OP_CONSTANT) {
            fprintf(fp, "  li %s, %d\n", registers->regList[resultRegNo]->name,
                    intercode->u.binOp.op1->u.value -
                        intercode->u.binOp.op2->u.value);
        }
        // 变量 常数
        else if (intercode->u.binOp.op1->kind != OP_CONSTANT &&
                 intercode->u.binOp.op2->kind == OP_CONSTANT) {
            int op1RegNo =
                allocReg(fp, varTable, registers, intercode->u.binOp.op1);
            fprintf(fp, "  addi %s, %s, %d\n",
                    registers->regList[resultRegNo]->name,
                    registers->regList[op1RegNo]->name,
                    -intercode->u.binOp.op2->u.value);
        }
        // 变量 变量
        else {
            int op1RegNo =
                allocReg(fp, varTable, registers, intercode->u.binOp.op1);
            int op2RegNo =
                allocReg(fp, varTable, registers, intercode->u.binOp.op2);
            fprintf(fp, "  sub %s, %s, %s\n",
                    registers->regList[resultRegNo]->name,
                    registers->regList[op1RegNo]->name,
                    registers->regList[op2RegNo]->name);
        }
    } else if (kind == IR_MUL) {
        int resultRegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.result);
        int op1RegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.op1);
        int op2RegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.op2);
        fprintf(fp, "  mul %s, %s, %s\n", registers->regList[resultRegNo]->name,
                registers->regList[op1RegNo]->name,
                registers->regList[op2RegNo]->name);
    } else if (kind == IR_DIV) {
        int resultRegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.result);
        int op1RegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.op1);
        int op2RegNo =
            allocReg(fp, varTable, registers, intercode->u.binOp.op2);
        fprintf(fp, "  div %s, %s\n", registers->regList[op1RegNo]->name,
                registers->regList[op2RegNo]->name);
        fprintf(fp, "  mflo %s\n", registers->regList[resultRegNo]->name);
    } else if (kind == IR_DEC) {
        // init 到全局变量里了
        // fprintf(fp, "  %s: .word %d\n", intercode->u.dec.op->u.name,
        //         intercode->u.dec.size);

    } else if (kind == IR_IF_GOTO) {
        char* relopName = intercode->u.ifGoto.relop->u.name;
        int xRegNo =
            allocReg(fp, varTable, registers, intercode->u.ifGoto.x);
        int yRegNo =
            allocReg(fp, varTable, registers, intercode->u.ifGoto.y);
        if (!strcmp(relopName, "=="))
            fprintf(fp, "  beq %s, %s, %s\n", registers->regList[xRegNo]->name,
                    registers->regList[yRegNo]->name,
                    intercode->u.ifGoto.z->u.name);
        else if (!strcmp(relopName, "!="))
            fprintf(fp, "  bne %s, %s, %s\n", registers->regList[xRegNo]->name,
                    registers->regList[yRegNo]->name,
                    intercode->u.ifGoto.z->u.name);
        else if (!strcmp(relopName, ">"))
            fprintf(fp, "  bgt %s, %s, %s\n", registers->regList[xRegNo]->name,
                    registers->regList[yRegNo]->name,
                    intercode->u.ifGoto.z->u.name);
        else if (!strcmp(relopName, "<"))
            fprintf(fp, "  blt %s, %s, %s\n", registers->regList[xRegNo]->name,
                    registers->regList[yRegNo]->name,
                    intercode->u.ifGoto.z->u.name);
        else if (!!strcmp(relopName, ">="))
            fprintf(fp, "  bge %s, %s, %s\n", registers->regList[xRegNo]->name,
                    registers->regList[yRegNo]->name,
                    intercode->u.ifGoto.z->u.name);
        else if (strcmp(relopName, "<="))
            fprintf(fp, "  ble %s, %s, %s\n", registers->regList[xRegNo]->name,
                    registers->regList[yRegNo]->name,
                    intercode->u.ifGoto.z->u.name);
    }
}

void pusha(FILE* fp){
    fprintf(fp, "  addi $sp, $sp, -72\n");
    for (int i = T0; i <= T9; i++) {
        fprintf(fp, "  sw %s, %d($sp)\n", registers->regList[i]->name,
                (i - T0) * 4);
    }
}

void popa(FILE* fp) {
    for (int i = T0; i <= T9; i++) {
        fprintf(fp, "  lw %s, %d($sp)\n", registers->regList[i]->name,
                (i - T0) * 4);
    }
    fprintf(fp, "  addi $sp, $sp, 72\n");
}