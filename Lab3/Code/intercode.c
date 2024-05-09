#include "semantic.h"
#include "intercode.h"

//intercode 双向链表
InterCodeList IR;

//hash table
extern SymbolTableEntry* hashTable[HASH_SIZE];

int interError = 0;

//func for intercode
//新建中间代码
void initIR() {
    IR = (InterCodeList)malloc(sizeof(struct InterCodeList_));
    IR->head = NULL;
    IR->cur = NULL;
    //IR->lastArrayName = NULL;
    IR->tempVarNum = 1;
    IR->labelNum = 1;
}
InterCode newInterCode(int kind, ...) {
    InterCode p = (InterCode)malloc(sizeof(struct InterCode_));
    assert(p != NULL);
    p->kind = kind;
    va_list vaList;
    assert(kind >= 0 && kind < 19);
    switch (kind) {
        case IR_LABEL:
        case IR_FUNCTION:
        case IR_GOTO:
        case IR_RETURN:
        case IR_ARG:
        case IR_PARAM:
        case IR_READ:
        case IR_WRITE:
            va_start(vaList, kind);
            p->u.oneOp.op = va_arg(vaList, Operand);
            break;
        case IR_ASSIGN:
        case IR_GET_ADDR:
        case IR_READ_ADDR:
        case IR_WRITE_ADDR:
        case IR_CALL:
            va_start(vaList, kind);
            p->u.assign.left = va_arg(vaList, Operand);
            p->u.assign.right = va_arg(vaList, Operand);
            break;
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            va_start(vaList, kind);
            p->u.binOp.result = va_arg(vaList, Operand);
            p->u.binOp.op1 = va_arg(vaList, Operand);
            p->u.binOp.op2 = va_arg(vaList, Operand);
            break;
        case IR_DEC:
            va_start(vaList, kind);
            p->u.dec.op = va_arg(vaList, Operand);
            p->u.dec.size = va_arg(vaList, int);
            break;
        case IR_IF_GOTO:
            va_start(vaList, kind);
            p->u.ifGoto.x = va_arg(vaList, Operand);
            p->u.ifGoto.relop = va_arg(vaList, Operand);
            p->u.ifGoto.y = va_arg(vaList, Operand);
            p->u.ifGoto.z = va_arg(vaList, Operand);
    }
    p->prev = NULL;
    p->next = NULL;
    return p;
}

//插入新中间代码
void insertInterCode(InterCode newCode) {
    //printf("debug\n");
    if (IR->head == NULL) {
        IR->head = newCode;
        IR->cur = newCode;
    } else {
        IR->cur->next = newCode;
        newCode->prev = IR->cur;
        IR->cur = newCode;
    }
}

void genInterCode(int kind, ...) {
    //printf("debug\n");
    va_list vaList;
    Operand temp = NULL;
    Operand result = NULL, op1 = NULL, op2 = NULL, relop = NULL;
    int size = 0;
    InterCode newCode = NULL;
    assert(kind >= 0 && kind < 19);
    switch (kind) {
        case IR_LABEL:
        case IR_FUNCTION:
        case IR_GOTO:
        case IR_RETURN:
        case IR_ARG:
        case IR_PARAM:
        case IR_READ:
        case IR_WRITE:
            va_start(vaList, kind);
            op1 = va_arg(vaList, Operand);
            if (op1->kind == OP_ADDRESS) {
                temp = newTemp();
                genInterCode(IR_READ_ADDR, temp, op1);
                op1 = temp;
            }
            newCode = newInterCode(kind, op1);
            insertInterCode(newCode);
            break;
        case IR_ASSIGN:
        case IR_GET_ADDR:
        case IR_READ_ADDR:
        case IR_WRITE_ADDR:
        case IR_CALL:
            va_start(vaList, kind);
            op1 = va_arg(vaList, Operand);
            op2 = va_arg(vaList, Operand);
            if (kind == IR_ASSIGN &&
                (op1->kind == OP_ADDRESS || op2->kind == OP_ADDRESS)) {
                if (op1->kind == OP_ADDRESS && op2->kind != OP_ADDRESS)
                    genInterCode(IR_WRITE_ADDR, op1, op2);
                else if (op2->kind == OP_ADDRESS && op1->kind != OP_ADDRESS)
                    genInterCode(IR_READ_ADDR, op1, op2);
                else {
                    temp = newTemp();
                    genInterCode(IR_READ_ADDR, temp, op2);
                    genInterCode(IR_WRITE_ADDR, op1, temp);
                }
            } else {
                newCode = newInterCode(kind, op1, op2);
                insertInterCode(newCode);
            }
            break;
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            va_start(vaList, kind);
            result = va_arg(vaList, Operand);
            op1 = va_arg(vaList, Operand);
            op2 = va_arg(vaList, Operand);
            if (op1->kind == OP_ADDRESS) {
                temp = newTemp();
                genInterCode(IR_READ_ADDR, temp, op1);
                op1 = temp;
            }
            if (op2->kind == OP_ADDRESS) {
                temp = newTemp();
                genInterCode(IR_READ_ADDR, temp, op2);
                op2 = temp;
            }
            newCode = newInterCode(kind, result, op1, op2);
            insertInterCode(newCode);
            break;
        case IR_DEC:
            va_start(vaList, kind);
            op1 = va_arg(vaList, Operand);
            size = va_arg(vaList, int);
            newCode = newInterCode(kind, op1, size);
            insertInterCode(newCode);
            break;
        case IR_IF_GOTO:
            va_start(vaList, kind);
            result = va_arg(vaList, Operand);
            relop = va_arg(vaList, Operand);
            op1 = va_arg(vaList, Operand);
            op2 = va_arg(vaList, Operand);
            newCode = newInterCode(kind, result, relop, op1, op2);
            insertInterCode(newCode);
            break;
    }
}

Operand newTemp() {
    char tempName[10] = {0};
    sprintf(tempName, "t%d", IR->tempVarNum);
    IR->tempVarNum++;
    Operand op = (Operand)malloc(sizeof(struct Operand_));
	op->kind = OP_VARIABLE;
    op->u.name=my_strdup(tempName);
    return op;
}

Operand newLabel() {
    char labelName[10] = {0};
    sprintf(labelName, "label%d", IR->labelNum);
    IR->labelNum++;
    Operand op = (Operand)malloc(sizeof(struct Operand_));
	op->kind = OP_LABEL;
    op->u.name=my_strdup(labelName);
    return op;
}

int getSize(Type type) {
    if (type == NULL)
        return 0;
    else if (type->kind == BASIC){
        if(type->u.basic==TYPE_INT){
            return 4;
        }else{
            return 8;
        }
    }
    else if (type->kind == ARRAY)
        return type->u.array.size * getSize(type->u.array.elem);
    else if (type->kind == STRUCTURE) {
        int size = 0;
        FieldList temp = type->u.structure;
        while (temp) {
            size += getSize(temp->type);
            temp = temp->tail;
        }
        return size;
    }
    return 0;
}

Arg newArg(Operand op) {
    Arg p = (Arg)malloc(sizeof(struct arg_));
    assert(p != NULL);
    p->op = op;
    p->next = NULL;
}

ArgList newArgList() {
    ArgList p = (ArgList)malloc(sizeof(struct argList_));
    assert(p != NULL);
    p->head = NULL;
    p->cur = NULL;
}

void addArg(ArgList argList, Arg arg) {
    if (argList->head == NULL) {
        argList->head = arg;
        argList->cur = arg;
    } else {
        argList->cur->next = arg;
        argList->cur = arg;
    }
}


//打印中间代码
void printOp(FILE* fp, Operand op) {
    assert(op != NULL);
    if (fp == NULL) {
        switch (op->kind) {
            case OP_CONSTANT:
                printf("#%d", op->u.value);
                break;
            case OP_VARIABLE:
            case OP_ADDRESS:
            case OP_LABEL:
            case OP_FUNCTION:
            case OP_RELOP:
                printf("%s", op->u.name);
                break;
        }
    } else {
        switch (op->kind) {
            case OP_CONSTANT:
                fprintf(fp, "#%d", op->u.value);
                break;
            case OP_VARIABLE:
            case OP_ADDRESS:
            case OP_LABEL:
            case OP_FUNCTION:
            case OP_RELOP:
                fprintf(fp, "%s", op->u.name);
                break;
        }
    }
}


void printInterCode(FILE* fp) {
    if(interError==1) return;
    for (InterCode cur = IR->head; cur != NULL; cur = cur->next) {
        assert(cur->kind >= 0 && cur->kind < 19);
        if (fp == NULL) {
            switch (cur->kind) {
                case IR_LABEL:
                    printf("LABEL ");
                    printOp(fp, cur->u.oneOp.op);
                    printf(" :");
                    break;
                case IR_FUNCTION:
                    printf("FUNCTION ");
                    printOp(fp, cur->u.oneOp.op);
                    printf(" :");
                    break;
                case IR_ASSIGN:
                    printOp(fp, cur->u.assign.left);
                    printf(" := ");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_ADD:
                    printOp(fp, cur->u.binOp.result);
                    printf(" := ");
                    printOp(fp, cur->u.binOp.op1);
                    printf(" + ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_SUB:
                    printOp(fp, cur->u.binOp.result);
                    printf(" := ");
                    printOp(fp, cur->u.binOp.op1);
                    printf(" - ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_MUL:
                    printOp(fp, cur->u.binOp.result);
                    printf(" := ");
                    printOp(fp, cur->u.binOp.op1);
                    printf(" * ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_DIV:
                    printOp(fp, cur->u.binOp.result);
                    printf(" := ");
                    printOp(fp, cur->u.binOp.op1);
                    printf(" / ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_GET_ADDR:
                    printOp(fp, cur->u.assign.left);
                    printf(" := &");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_READ_ADDR:
                    printOp(fp, cur->u.assign.left);
                    printf(" := *");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_WRITE_ADDR:
                    printf("*");
                    printOp(fp, cur->u.assign.left);
                    printf(" := ");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_GOTO:
                    printf("GOTO ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_IF_GOTO:
                    printf("IF ");
                    printOp(fp, cur->u.ifGoto.x);
                    printf(" ");
                    printOp(fp, cur->u.ifGoto.relop);
                    printf(" ");
                    printOp(fp, cur->u.ifGoto.y);
                    printf(" GOTO ");
                    printOp(fp, cur->u.ifGoto.z);
                    break;
                case IR_RETURN:
                    printf("RETURN ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_DEC:
                    printf("DEC ");
                    printOp(fp, cur->u.dec.op);
                    printf(" ");
                    printf("%d", cur->u.dec.size);
                    break;
                case IR_ARG:
                    printf("ARG ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_CALL:
                    printOp(fp, cur->u.assign.left);
                    printf(" := CALL ");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_PARAM:
                    printf("PARAM ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_READ:
                    printf("READ ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_WRITE:
                    printf("WRITE ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
            }
            printf("\n");
        } else {
            switch (cur->kind) {
                case IR_LABEL:
                    fprintf(fp, "LABEL ");
                    printOp(fp, cur->u.oneOp.op);
                    fprintf(fp, " :");
                    break;
                case IR_FUNCTION:
                    fprintf(fp, "FUNCTION ");
                    printOp(fp, cur->u.oneOp.op);
                    fprintf(fp, " :");
                    break;
                case IR_ASSIGN:
                    printOp(fp, cur->u.assign.left);
                    fprintf(fp, " := ");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_ADD:
                    printOp(fp, cur->u.binOp.result);
                    fprintf(fp, " := ");
                    printOp(fp, cur->u.binOp.op1);
                    fprintf(fp, " + ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_SUB:
                    printOp(fp, cur->u.binOp.result);
                    fprintf(fp, " := ");
                    printOp(fp, cur->u.binOp.op1);
                    fprintf(fp, " - ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_MUL:
                    printOp(fp, cur->u.binOp.result);
                    fprintf(fp, " := ");
                    printOp(fp, cur->u.binOp.op1);
                    fprintf(fp, " * ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_DIV:
                    printOp(fp, cur->u.binOp.result);
                    fprintf(fp, " := ");
                    printOp(fp, cur->u.binOp.op1);
                    fprintf(fp, " / ");
                    printOp(fp, cur->u.binOp.op2);
                    break;
                case IR_GET_ADDR:
                    printOp(fp, cur->u.assign.left);
                    fprintf(fp, " := &");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_READ_ADDR:
                    printOp(fp, cur->u.assign.left);
                    fprintf(fp, " := *");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_WRITE_ADDR:
                    fprintf(fp, "*");
                    printOp(fp, cur->u.assign.left);
                    fprintf(fp, " := ");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_GOTO:
                    fprintf(fp, "GOTO ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_IF_GOTO:
                    fprintf(fp, "IF ");
                    printOp(fp, cur->u.ifGoto.x);
                    fprintf(fp, " ");
                    printOp(fp, cur->u.ifGoto.relop);
                    fprintf(fp, " ");
                    printOp(fp, cur->u.ifGoto.y);
                    fprintf(fp, " GOTO ");
                    printOp(fp, cur->u.ifGoto.z);
                    break;
                case IR_RETURN:
                    fprintf(fp, "RETURN ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_DEC:
                    fprintf(fp, "DEC ");
                    printOp(fp, cur->u.dec.op);
                    fprintf(fp, " ");
                    fprintf(fp, "%d", cur->u.dec.size);
                    break;
                case IR_ARG:
                    fprintf(fp, "ARG ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_CALL:
                    printOp(fp, cur->u.assign.left);
                    fprintf(fp, " := CALL ");
                    printOp(fp, cur->u.assign.right);
                    break;
                case IR_PARAM:
                    fprintf(fp, "PARAM ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_READ:
                    fprintf(fp, "READ ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
                case IR_WRITE:
                    fprintf(fp, "WRITE ");
                    printOp(fp, cur->u.oneOp.op);
                    break;
            }
            fprintf(fp, "\n");
        }
    }
}




//generate intercode
//Program     : ExtDefList
void get_intercode(TreeNode *root)
{
    if(root==NULL) return;
    if (interError) return;
    initIR();
    if(!strcmp("ExtDefList" , root->children[0]->name)){
        translateExtDefList(root ->children[0]);
    }
}

// ExtDefList  : ExtDef ExtDefList  
//             |  /* empty */       
//             ;
void translateExtDefList(TreeNode *node)
{
    if(node == NULL )return;
    if (interError) return;
    if(node->childrenCount ==2){
        translateExtDef(node->children[0]);
        translateExtDefList(node->children[1]);  
    }
}

// ExtDef      : Specifier ExtDecList SEMI  
//             | Specifier SEMI 
//             | Specifier FunDec CompSt   
void translateExtDef(TreeNode *node)
{
    if(node == NULL )return;
    if (interError) return;
	//translateSpecifier(node->children[0]);
    //Specifier ExtDecList SEMI  
    if(!strcmp(node->children[1]->name,"ExtDecList")){
        //不涉及中间代码生成
        //tExtDecList(node->children[1]);
    }
    else if(!strcmp(node->children[1]->name,"FunDec")) 
    //Specifier FunDec CompSt 
    //FunDec是函数头，CompSt表示函数体
    {
        if(!strcmp(node->children[2]->name,"CompSt")){
            translateFunDec(node->children[1]);
            //printf("debug\n");
            translateCompSt(node->children[2]);
        }
    }
    //Specifier SEMI
}

//不涉及中间代码生成
/*
// Specifier   : TYPE            
//             | StructSpecifier 
//             ;
Type translateSpecifier(TreeNode *node)
{
    // TYPE
    if(!strcmp(node->children[0]->name,"TYPE")){
		Type spec = (Type)malloc(sizeof(struct Type_));
        spec->kind = BASIC;
        if(!strcmp(node->children[0]->value,"float"))
            spec->u.basic=TYPE_FLOAT;
        else
            spec->u.basic=TYPE_INT;
        return spec;
    }
    else{
		Type t = translateStructSpecifier(node->children[0]);
		if(t == NULL) return NULL;
        Type t2 = (Type)malloc(sizeof(struct Type_));
        t2->kind=STRUCTURE;
        t2->u.structure=t->u.structure;
        return t2;
    }
}


// StructSpecifier : STRUCT OptTag LC DefList RC
//                 | STRUCT Tag               
//                 ;
Type translateStructSpecifier(TreeNode *node)
{
    Type spec = (Type)malloc(sizeof(struct Type_));
    spec->kind = STRUCTURE;
    //定义结构体
    if(node->children[1] !=NULL && !strcmp(node -> children[1]->name ,"Tag"))
    {   //StructSpecifier → STRUCT Tag
        //tag->id
		TreeNode * Tag = node->children[1];
        TreeNode * ID = Tag->children[0];
        char *str = ID->value;
        //找结构体类型
        SymbolTableEntry* entry = find(str,2);
		return entry->type;
    }

    //STRUCT (OptTag) LC DefList RC 定义结构体类型
	return NULL;

}
*/

// FunDec      : ID LP VarList RP             
//             | ID LP RP                             
//             ;
void translateFunDec(TreeNode *node)
{
    if(node==NULL) return;
    if (interError) return;
	//生成函数定义中间代码
	Operand op = (Operand)malloc(sizeof(struct Operand_));
	op->kind = OP_FUNCTION;
    //printf("debug1\n");
    //if(op==NULL){
    //    printf("debug2\n");
    //}
    //if(node->children[0]==NULL){
    //    printf("debug3\n");
    //}
    //printf("value: %s\n", node->children[0]->value);
    op->u.name=my_strdup(node->children[0]->value);
	//strcpy(op->u.name,node->children[0]->value);
    //printf("debug2\n");
    genInterCode(IR_FUNCTION, op);
    //printf("debug\n");
	
	char * funcname = node->children[0]->value;
	SymbolTableEntry* entry = find(funcname,1);
	FieldList param = entry->type->u.func.params;
	for(int i=0; i<entry->type->u.func.paramNum;i++)
	{
		//形参的中间代码
        if(param->type->kind==STRUCTURE){
            interError=1;
            return;
        }
        Operand paramop = (Operand)malloc(sizeof(struct Operand_));
        paramop->kind = OP_VARIABLE;
        paramop->u.name=my_strdup(param->name);
        genInterCode(IR_PARAM, paramop);
	    param = param->tail;
	}
}

// CompSt      : LC DefList StmtList RC                   
//             ;
void translateCompSt(TreeNode* node)
{
    if(node==NULL) return;
    if (interError) return;
    // CompSt -> LC DefList StmtList RC
    TreeNode* deflist = node->children[1];
    //printf("name: %s\n", deflist->name);
    if (deflist!=NULL && !strcmp(deflist->name, "DefList")) {
        translateDefList(deflist);
    }
    TreeNode* stmtlist = node->children[2];
    if (stmtlist!=NULL && !strcmp(stmtlist->name, "StmtList")) {
        translateStmtList(stmtlist);
    }
}

void translateDefList(TreeNode* node) {
    // DefList -> Def DefList
    //          | 
    if(node==NULL) return;
    if (interError) return;
    translateDef(node->children[0]);
    translateDefList(node->children[1]);
}

void translateDef(TreeNode* node) {
    if(node==NULL) return;
    if (interError) return;
    // Def -> Specifier DecList SEMI
    translateDecList(node->children[1]);
}

// DecList : Dec                 
//         | Dec COMMA DecList        
//         ;
void translateDecList(TreeNode* node)
{
    if(node==NULL) return;
    if (interError) return;
    translateDec(node->children[0]);
    if(node->childrenCount == 3)
    {
        translateDecList(node->children[2]);
    }
}

void translateDec(TreeNode* node) {
    if(node==NULL) return;
    if (interError) return;
    // Dec -> VarDec
    //      | VarDec ASSIGNOP Exp

    // Dec -> VarDec
    if (node->childrenCount==1) {
        translateVarDec(node->children[0], NULL);
    }
    // Dec -> VarDec ASSIGNOP Exp
    else {
        Operand t1 = newTemp();
        translateVarDec(node->children[0], t1);

        Operand t2 = newTemp();
        translateExp(node->children[2], t2);

        genInterCode(IR_ASSIGN, t1, t2);
    }
}

void translateVarDec(TreeNode* node, Operand place) {
    if(node==NULL) return;
    if (interError) return;
    // VarDec -> ID
    //         | VarDec LB INT RB
    if (!strcmp(node->children[0]->name, "ID")) {
        SymbolTableEntry* entry = find(node->children[0]->value,2);
        Type type = entry->type;
        if (type->kind == BASIC) {
            if (place) {
                //IR->tempVarNum--;
                place->kind=OP_VARIABLE;
                place->u.name=my_strdup(entry->name);
            }
        } else if (type->kind == ARRAY) {
            // 选做：可能出现高维数组情况
            Operand op = (Operand)malloc(sizeof(struct Operand_));
	        op->kind = OP_VARIABLE;
            op->u.name=my_strdup(entry->name);
            genInterCode(IR_DEC, op, getSize(type));
        } else if (type->kind == STRUCTURE) {
            // 不会出现类型为结构体的变量
            interError=1;
            //printf("Cannot translate: Code contains variables of structure type.\n");
            return;
        }
    } else {
        translateVarDec(node->children[0], place);
    }
}

void translateStmtList(TreeNode* node) {
    // StmtList -> Stmt StmtList
    //           | 
    if(node==NULL) return;
    if (interError) return;
    translateStmt(node->children[0]);
    translateStmtList(node->children[1]);
}

void translateStmt(TreeNode* node) {
    if(node==NULL) return;
    if (interError) return;
    // Stmt -> Exp SEMI
    //       | CompSt
    //       | RETURN Exp SEMI
    //       | IF LP Exp RP Stmt
    //       | IF LP Exp RP Stmt ELSE Stmt
    //       | WHILE LP Exp RP Stmt

    // Stmt -> Exp SEMI
    if (!strcmp(node->children[0]->name, "Exp")) {
        translateExp(node->children[0], NULL);
    }

    // Stmt -> CompSt
    else if (!strcmp(node->children[0]->name, "CompSt")) {
        translateCompSt(node->children[0]);
    }

    // Stmt -> RETURN Exp SEMI
    else if (!strcmp(node->children[0]->name, "RETURN")) {
        Operand t1 = newTemp();
        translateExp(node->children[1], t1);
        genInterCode(IR_RETURN, t1);
    }

    // Stmt -> IF LP Exp RP Stmt
    else if (!strcmp(node->children[0]->name, "IF")) {
        TreeNode* exp = node->children[2];
        TreeNode* stmt = node->children[4];
        Operand label1 = newLabel();
        Operand label2 = newLabel();

        translateCond(exp, label1, label2);
        genInterCode(IR_LABEL, label1);
        translateStmt(stmt);
        if (node->childrenCount==5) {
            genInterCode(IR_LABEL, label2);
        }
        // Stmt -> IF LP Exp RP Stmt ELSE Stmt
        else {
            Operand label3 = newLabel();
            genInterCode(IR_GOTO, label3);
            genInterCode(IR_LABEL, label2);
            translateStmt(node->children[6]);
            genInterCode(IR_LABEL, label3);
        }

    }

    // Stmt -> WHILE LP Exp RP Stmt
    else if (!strcmp(node->children[0]->name, "WHILE")) {
        Operand label1 = newLabel();
        Operand label2 = newLabel();
        Operand label3 = newLabel();

        genInterCode(IR_LABEL, label1);
        translateCond(node->children[2], label2, label3);
        genInterCode(IR_LABEL, label2);
        translateStmt(node->children[4]);
        genInterCode(IR_GOTO, label1);
        genInterCode(IR_LABEL, label3);
    }
}

void translateCond(TreeNode* node, Operand labelTrue, Operand labelFalse) {
    if(node==NULL) return;
    if (interError) return;
    // Exp -> Exp AND Exp
    //      | Exp OR Exp
    //      | Exp RELOP Exp
    //      | NOT Exp

    // Exp -> NOT Exp
    if (!strcmp(node->children[0]->name, "NOT")) {
        translateCond(node->children[1], labelFalse, labelTrue);
    }
    // Exp -> Exp RELOP Exp
    else if (!strcmp(node->children[1]->name, "RELOP")) {
        Operand t1 = newTemp();
        Operand t2 = newTemp();
        translateExp(node->children[0], t1);
        translateExp(node->children[2], t2);

        Operand relop = (Operand)malloc(sizeof(struct Operand_));
	    relop->kind = OP_RELOP;
        relop->u.name=my_strdup(node->children[1]->value);

        if (t1->kind == OP_ADDRESS) {
            Operand temp = newTemp();
            genInterCode(IR_READ_ADDR, temp, t1);
            t1 = temp;
        }
        if (t2->kind == OP_ADDRESS) {
            Operand temp = newTemp();
            genInterCode(IR_READ_ADDR, temp, t2);
            t2 = temp;
        }

        genInterCode(IR_IF_GOTO, t1, relop, t2, labelTrue);
        genInterCode(IR_GOTO, labelFalse);
    }
    // Exp -> Exp AND Exp
    else if (!strcmp(node->children[1]->name, "AND")) {
        Operand label1 = newLabel();
        translateCond(node->children[0], label1, labelFalse);
        genInterCode(IR_LABEL, label1);
        translateCond(node->children[2], labelTrue, labelFalse);
    }
    // Exp -> Exp OR Exp
    else if (!strcmp(node->children[1]->name, "OR")) {
        Operand label1 = newLabel();
        translateCond(node->children[0], labelTrue, label1);
        genInterCode(IR_LABEL, label1);
        translateCond(node->children[2], labelTrue, labelFalse);
    }
    // other cases(!=0)
    else {
        Operand t1 = newTemp();
        translateExp(node, t1);

        Operand t2 = (Operand)malloc(sizeof(struct Operand_));
	    t2->kind = OP_CONSTANT;
	    t2->u.value = 0;
        Operand relop = (Operand)malloc(sizeof(struct Operand_));
	    relop->kind = OP_RELOP;
	    relop->u.name = "!=";

        if (t1->kind == OP_ADDRESS) {
            Operand temp = newTemp();
            genInterCode(IR_READ_ADDR, temp, t1);
            t1 = temp;
        }
        genInterCode(IR_IF_GOTO, t1, relop, t2, labelTrue);
        genInterCode(IR_GOTO, labelFalse);
    }
}

void translateExp(TreeNode* node, Operand place) {
    if(node==NULL) return;
    if (interError) return;
    // Exp -> Exp ASSIGNOP Exp
    //      | Exp AND Exp
    //      | Exp OR Exp
    //      | Exp RELOP Exp
    //      | Exp PLUS Exp
    //      | Exp MINUS Exp
    //      | Exp STAR Exp
    //      | Exp DIV Exp
    //      | LP Exp RP
    //      | MINUS Exp
    //      | NOT Exp
    //      | ID LP Args RP
    //      | ID LP RP
    //      | Exp LB Exp RB
    //      | Exp DOT ID
    //      | ID
    //      | INT
    //      | FLOAT

    // Exp -> LP Exp RP
    if (!strcmp(node->children[0]->name, "LP"))
        translateExp(node->children[1], place);

    else if (!strcmp(node->children[0]->name, "Exp") ||
             !strcmp(node->children[0]->name, "NOT")) {
        // 条件表达式和基本表达式
        if (strcmp(node->children[1]->name, "LB") &&
            strcmp(node->children[1]->name, "DOT")) {
            // Exp -> Exp AND Exp
            //      | Exp OR Exp
            //      | Exp RELOP Exp
            //      | NOT Exp
            if (!strcmp(node->children[1]->name, "AND") ||
                !strcmp(node->children[1]->name, "OR") ||
                !strcmp(node->children[1]->name, "RELOP") ||
                !strcmp(node->children[0]->name, "NOT")) {
                Operand label1 = newLabel();
                Operand label2 = newLabel();
                Operand true_num = (Operand)malloc(sizeof(struct Operand_));
	            true_num->kind = OP_CONSTANT;
	            true_num->u.value = 1;
                Operand false_num = (Operand)malloc(sizeof(struct Operand_));
	            false_num->kind = OP_CONSTANT;
	            false_num->u.value = 0;
                genInterCode(IR_ASSIGN, place, false_num);
                translateCond(node, label1, label2);
                genInterCode(IR_LABEL, label1);
                genInterCode(IR_ASSIGN, place, true_num);
                //这里缺一行？
                //genInterCode(IR_LABEL, label2);

            } else {
                // Exp -> Exp ASSIGNOP Exp
                if (!strcmp(node->children[1]->name, "ASSIGNOP")) {
                    Operand t2 = newTemp();
                    translateExp(node->children[2], t2);
                    Operand t1 = newTemp();
                    //printf("debug\n");
                    translateExp(node->children[0], t1);
                    //printf("debug1\n");
                    genInterCode(IR_ASSIGN, t1, t2);
                } else {
                    Operand t1 = newTemp();
                    translateExp(node->children[0], t1);
                    Operand t2 = newTemp();
                    translateExp(node->children[2], t2);
                    // Exp -> Exp PLUS Exp
                    if (!strcmp(node->children[1]->name, "PLUS")) {
                        genInterCode(IR_ADD, place, t1, t2);
                    }
                    // Exp -> Exp MINUS Exp
                    else if (!strcmp(node->children[1]->name, "MINUS")) {
                        genInterCode(IR_SUB, place, t1, t2);
                    }
                    // Exp -> Exp STAR Exp
                    else if (!strcmp(node->children[1]->name, "STAR")) {
                        genInterCode(IR_MUL, place, t1, t2);
                    }
                    // Exp -> Exp DIV Exp
                    else if (!strcmp(node->children[1]->name, "DIV")) {
                        genInterCode(IR_DIV, place, t1, t2);
                    }
                }
            }

        }
        // 数组和结构体访问
        else {
            // Exp -> Exp LB Exp RB
            if (!strcmp(node->children[1]->name, "LB")) {
                //数组,可以是多维数组
                //printf("debug\n");
                TreeNode* tmp_exp=node;
                int num=0;
                while(tmp_exp->childrenCount!=1){
                    num++;
                    tmp_exp=tmp_exp->children[0];
                }
                TreeNode* id=tmp_exp->children[0];
                //printf("num: %d\n", num);
                SymbolTableEntry* entry=find(id->value, 2);
                Type entry_type=entry->type;
                //printf("debug\n");

                Operand base = newTemp();
                translateExp(tmp_exp, base);
                //Operand target = newTemp();
                genInterCode(IR_GET_ADDR, place, base);

                TreeNode* exp=node;
                //Operand offset = newTemp();
                int i=0;
                while(exp->childrenCount!=1){
                    Type temp_entry_type=entry_type;
                    //printf("i: %d\n", i);
                    i++;
                    Operand idx = newTemp();
                    translateExp(exp->children[2], idx);
                    Operand width = (Operand)malloc(sizeof(struct Operand_));
	                width->kind = OP_CONSTANT;
                    for(int j=num-i;j>=0;j--){
                        temp_entry_type=temp_entry_type->u.array.elem;
                    }
                    //printf("debug1\n");
                    //printf("type: %d\n", temp_entry_type->kind);
	                width->u.value = getSize(temp_entry_type);
                    //printf("width: %d\n", width->u.value);
                    //printf("debug2\n");
                    Operand temp = newTemp();
                    genInterCode(IR_MUL, temp, idx, width);
                    genInterCode(IR_ADD, place, place, temp);
                    exp=exp->children[0];
                    //printf("debug3\n");
                }

                //genInterCode(IR_ADD, place, target, );
                place->kind = OP_ADDRESS;
                //genInterCode = 
                //printf("debug1\n");

            }
            // Exp -> Exp DOT ID
            else {
                //结构体,不考虑
                interError = 1;
                return;
                /*
                Operand temp = newTemp();
                translateExp(node->children[0], temp);
                // 两种情况，Exp直接为一个变量，则需要先取址，若Exp为数组或者多层结构体访问或结构体形参，则target会被填成地址，可以直接用。
                Operand target;

                if (temp->kind == OP_ADDRESS) {
                    target = newOperand(temp->kind, temp->u.name);
                    // target->isAddr = TRUE;
                } else {
                    target = newTemp();
                    genInterCode(IR_GET_ADDR, target, temp);
                }

                pOperand id = newOperand(
                    OP_VARIABLE, newString(node->child->next->next->val));
                int offset = 0;
                pItem item = searchTableItem(table, temp->u.name);
                //结构体数组，temp是临时变量，查不到表，需要用处理数组时候记录下的数组名查表
                if (item == NULL) {
                    item = searchTableItem(table, interCodeList->lastArrayName);
                }

                pFieldList tmp;
                // 结构体数组 eg: a[5].b
                if (item->field->type->kind == ARRAY) {
                    tmp = item->field->type->u.array.elem->u.structure.field;
                }
                // 一般结构体
                else {
                    tmp = item->field->type->u.structure.field;
                }
                // 遍历获得offset
                while (tmp) {
                    if (!strcmp(tmp->name, id->u.name)) break;
                    offset += getSize(tmp->type);
                    tmp = tmp->tail;
                }

                pOperand tOffset = newOperand(OP_CONSTANT, offset);
                if (place) {
                    genInterCode(IR_ADD, place, target, tOffset);
                    // 为了处理结构体里的数组把id名通过place回传给上层
                    setOperand(place, OP_ADDRESS, (void*)newString(id->u.name));
                    // place->isAddr = TRUE;
                }
                */
            }
        }
    }

    //单目运算符
    // Exp -> MINUS Exp
    else if (!strcmp(node->children[0]->name, "MINUS")) {
        Operand t1 = newTemp();
        translateExp(node->children[1], t1);
        Operand zero = (Operand)malloc(sizeof(struct Operand_));
	    zero->kind = OP_CONSTANT;
	    zero->u.value = 0;
        genInterCode(IR_SUB, place, zero, t1);
    }

    // Exp -> ID LP Args RP
    //		| ID LP RP
    else if (!strcmp(node->children[0]->name, "ID") && node->childrenCount>1) {
        Operand funcTemp = (Operand)malloc(sizeof(struct Operand_));
	    funcTemp->kind = OP_FUNCTION;
        funcTemp->u.name=my_strdup(node->children[0]->value);
        // Exp -> ID LP Args RP
        if (!strcmp(node->children[2]->name, "Args")) {
            ArgList argList = newArgList();
            translateArgs(node->children[2], argList);
            if (!strcmp(node->children[0]->value, "write")) {
                genInterCode(IR_WRITE, argList->head->op);
            } else {
                Arg argTemp = argList->head;
                while (argTemp) {
                    //printf("debug\n");
                    //printf("argtemp_op: %d\n", argTemp->op->kind);
                    if (argTemp->op->kind == OP_VARIABLE) {
                        SymbolTableEntry* item = find(argTemp->op->u.name, 2);
                        //printf("debug\n");

                        // 一维数组作为参数需要传址
                        if (item && item->type->kind == ARRAY) {
                            Operand varTemp = newTemp();
                            genInterCode(IR_GET_ADDR, varTemp, argTemp->op);
                            Operand varTempCopy = (Operand)malloc(sizeof(struct Operand_));
	                        varTempCopy->kind = OP_VARIABLE;
                            varTempCopy->u.name=my_strdup(varTemp->u.name);
                            // varTempCopy->isAddr = TRUE;
                            genInterCode(IR_ARG, varTempCopy);
                        }
                    }
                    // 一般参数直接传值
                    else {
                        genInterCode(IR_ARG, argTemp->op);
                    }
                    argTemp = argTemp->next;
                }
                if (place) {
                    genInterCode(IR_CALL, place, funcTemp);
                } else {
                    Operand temp = newTemp();
                    genInterCode(IR_CALL, temp, funcTemp);
                }
            }
        }
        // Exp -> ID LP RP
        else {
            if (!strcmp(node->children[0]->value, "read")) {
                genInterCode(IR_READ, place);
            } else {
                if (place) {
                    genInterCode(IR_CALL, place, funcTemp);
                } else {
                    Operand temp = newTemp();
                    genInterCode(IR_CALL, temp, funcTemp);
                }
            }
        }
    }
    // Exp -> ID
    else if (!strcmp(node->children[0]->name, "ID")) {
        //SymbolTableEntry* item = find(node->children[0]->value, 2);
        // 根据讲义，数组做参数时传引用
        IR->tempVarNum--;
        //printf("isArg: %d\n", item->isArg);
        //if (item->isArg && item->type->kind == ARRAY) {
        //    printf("isArg: %d\n", item->isArg);
        //    printf("name1: %s\n", item->name);
	    //    place->kind = OP_ADDRESS;
        //    place->u.name=my_strdup(node->children[0]->value);
            // place->isAddr = TRUE;
        //}
        // 变量处理
        //else {
            place->kind = OP_VARIABLE;
            place->u.name=my_strdup(node->children[0]->value);
        //}

        // pOperand t1 = newOperand(OP_VARIABLE, id_name->field->name);
        // genInterCode(IR_ASSIGN, place, t1);
    } else {
        // // Exp -> FLOAT
        // 无浮点数常数
        // if (!strcmp(node->children[0]->name, "FLOAT")) {
        //     pOperand t1 = newOperand(OP_CONSTANT, node->child->val);
        //     genInterCode(IR_ASSIGN, place, t1);
        // }

        // Exp -> INT
        IR->tempVarNum--;
        place->kind = OP_CONSTANT;
        place->u.value=atoi(node->children[0]->value);
        // pOperand t1 = newOperand(OP_CONSTANT, node->child->val);
        // genInterCode(IR_ASSIGN, place, t1);
    }
}

void translateArgs(TreeNode* node, ArgList argList) {
    assert(node != NULL);
    assert(argList != NULL);
    if (interError) return;
    // Args -> Exp COMMA Args
    //       | Exp

    // Args -> Exp
    Arg temp = newArg(newTemp());
    translateExp(node->children[0], temp->op);
    //printf("arg_op: %d\n", temp->op->kind);
    //printf("name2: %s\n", temp->op->u.name);

    if (temp->op->kind == OP_VARIABLE) {
        SymbolTableEntry* item = find(temp->op->u.name, 2);
        //不考虑结构体传参
        if (item && item->type->kind == STRUCTURE) {
            interError = 1;
            return;
        }
    }
    addArg(argList, temp);

    // Args -> Exp COMMA Args
    if (node->childrenCount>1) {
        translateArgs(node->children[2], argList);
    }
}

