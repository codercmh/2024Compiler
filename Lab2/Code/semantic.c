#include "semantic.h"
#include "tree.h"


SymbolTableEntry* hashTable[HASH_SIZE];


void initHashTable() {
    for (int i = 0; i < HASH_SIZE; i++) {
        hashTable[i] = NULL;
    }
}

unsigned int hash_pjw(char* name){
    unsigned int val = 0, i;
    for (; *name; ++name){
        val = (val << 2) + *name;
        if (i = val & ~HASH_SIZE) val = (val ^ (i >> 12)) & HASH_SIZE;
    }
    return val;
}

//函数可以与变量重名
void insert(SymbolTableEntry* entry) {
    if(entry->name==NULL) return;
    unsigned int index = hash_pjw(entry->name) % HASH_SIZE;
    entry->next = hashTable[index];
    hashTable[index] = entry;
}

//function:type=1   others:type=2
//函数可以与变量重名
SymbolTableEntry* find(char* name, int type) {
    if(name==NULL) return NULL;
    unsigned int index = hash_pjw(name) % HASH_SIZE;
    SymbolTableEntry* entry = hashTable[index];
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            if((type==1&&entry->type->kind==FUNC) || (type==2&&entry->type->kind!=FUNC)){
                return entry;
            }
        }
        entry = entry->next;
    }
    return NULL;
}

//match:return=1    mismatch:return=0
int typeMatch(Type typea, Type typeb){
    if(typea==NULL||typeb==NULL) return 0;
    if(typea->kind!=typeb->kind){
        return 0;
    }
    if(typea->kind==BASIC){
        if(typea->u.basic==typeb->u.basic){
            return 1;
        }else{
            return 0;
        }
    }else if(typea->kind==ARRAY){
        //只要数组的基类型和维数相同我们即认为类型是匹配的
        return typeMatch(typea->u.array.elem, typeb->u.array.elem);
    }else if(typea->kind==STRUCTURE || typea->kind==STRUCT_DEF){
        //针对结构体中的每个域逐个进行类型比较（结构等价)
        FieldList fielda=typea->u.structure;
        FieldList fieldb=typeb->u.structure;
        if(fielda==NULL&&fieldb==NULL){
            return 1;
        }else if(fielda==NULL&&fieldb!=NULL || fieldb==NULL&&fielda!=NULL){
            return 0;
        }
        while(fielda!=NULL){
            if(fieldb==NULL){
                return 0;
            }
            if(typeMatch(fielda->type,fieldb->type)==0){
                return 0;
            }
            fielda=fielda->tail;
            fieldb=fieldb->tail;
        }
        if(fielda==NULL&&fieldb==NULL){
            return 1;
        }
    }else if(typea->kind==FUNC){
        //函数等价判断
        if(typea->u.func.paramNum!=typeb->u.func.paramNum){
            return 0;
        }
        if(typeMatch(typea->u.func.returnType, typeb->u.func.returnType)==0){
            return 0;
        }
        FieldList paramsa=typea->u.func.params;
        FieldList paramsb=typeb->u.func.params;
        for(int i=0;i<typea->u.func.paramNum;i++){
            if(typeMatch(paramsa->type, paramsb->type)==0){
                return 0;
            }
            paramsa=paramsa->tail;
            paramsb=paramsb->tail;
        }
        return 1;
    }
    return 0;
}


//entry -> FieldList
FieldList createFieldFromEntry(SymbolTableEntry* entry) {
    if (entry == NULL) {
        return NULL;
    }
    FieldList field = (FieldList)malloc(sizeof(struct FieldList_));
    if (field == NULL) {
        return NULL;  // Handle allocation failure
    }
    field->name = my_strdup(entry->name); 
    //printf("%s\n",field->name);
    field->type = entry->type;         
    field->tail = createFieldFromEntry(entry->next);                
    return field;
}



//semantic analysis
//Program → ExtDefList
void semantic_analysis(TreeNode* root){
    if(root==NULL) return;
    initHashTable();
    if(!strcmp(root->children[0]->name, "ExtDefList")){
        ExtDefList(root->children[0]);
    }
}

//ExtDefList → ExtDef ExtDefList
// | 
void ExtDefList(TreeNode* node){
    if(node==NULL) return;
    if(node->childrenCount==2){
        ExtDef(node->children[0]);
        ExtDefList(node->children[1]);
    }
}

//ExtDef → Specifier ExtDecList SEMI
// | Specifier SEMI
// | Specifier FunDec CompSt
void ExtDef(TreeNode* node){
    if(node->childrenCount==0) return;
    Type type=Specifier(node->children[0]);
    if(!strcmp(node->children[1]->name, "ExtDecList")){
        ExtDecList(node->children[1], type);
    }else if(!strcmp(node->children[1]->name, "FunDec")){
        FunDec(node->children[1], type);
        CompSt(node->children[2], type);
    }
}

//ExtDecList → VarDec
// | VarDec COMMA ExtDecList
//定义全局变量
void ExtDecList(TreeNode* node, Type type){
    SymbolTableEntry* entry=VarDec(node->children[0], type);
    if(find(entry->name, 2)!=NULL){
        printf("Error type 3 at Line %d: Redefined variable %s.\n",node->children[0]->lineNo,entry->name);
    }else{
        insert(entry);
    }
    if(node->childrenCount==3){
        ExtDecList(node->children[2], type);
    }
}

//Specifier → TYPE
// | StructSpecifier
Type Specifier(TreeNode* node){
    if(!strcmp(node->children[0]->name, "TYPE")){
        Type type=(Type)malloc(sizeof(struct Type_));
        type->kind=BASIC;
        if(!strcmp(node->children[0]->value, "float")){
            type->u.basic=TYPE_FLOAT;
        }else if(!strcmp(node->children[0]->value, "int")){
            type->u.basic=TYPE_INT;
        }
        return type;
    }else if(!strcmp(node->children[0]->name, "StructSpecifier")){
        //这里注意回传新定义的结构体类型时应该把类型改为STRUCTURE
        Type type=StructSpecifier(node->children[0]);
        Type type_return=(Type)malloc(sizeof(struct Type_));
        type_return->kind=STRUCTURE;
        type_return->u.structure=type->u.structure;
        return type_return;
    }
}

//StructSpecifier → STRUCT OptTag LC DefList RC
// | STRUCT Tag
//OptTag → ID
// | 
//Tag → ID
Type StructSpecifier(TreeNode* node){
    if(node->children[1]!=NULL && !strcmp(node->children[1]->name, "Tag")){
        //定义结构体
        Type type=(Type)malloc(sizeof(struct Type_));
        type->kind=STRUCTURE;
        char *str=node->children[1]->children[0]->value;
        int lineNo=node->children[1]->children[0]->lineNo;
        SymbolTableEntry* entry=find(str,2);
        if(entry==NULL){
            printf("Error type 17 at Line %d: Undefined structure %s.\n", lineNo, str);
            type->u.structure=NULL;
        }else{
            type->u.structure=entry->type->u.structure;
        }
        //printf("field name %s\n",type->u.structure->name);
        //printf("head %p\n",type->u.structure);
        //printf("next field %p\n",type->u.structure->tail);
        return type;
    }

    //新结构体类型
    Type type=(Type)malloc(sizeof(struct Type_));
    type->kind=STRUCT_DEF;
    //此处不调用DefList()，单独处理
    TreeNode* defList=node->children[3];
    type->u.structure=NULL;
    while(defList!=NULL){
        TreeNode* def=defList->children[0];
        Type type0=Specifier(def->children[0]);
        TreeNode* decList=def->children[1];
        while(1){
            TreeNode* dec=decList->children[0];
            TreeNode* varDec=dec->children[0];
            SymbolTableEntry* entry=VarDec(varDec, type0);
            if(dec->childrenCount==3){
                printf("Error type 15 at Line %d: Variable %s is initialized in struct.\n", varDec->lineNo, entry->name);
            }
            FieldList params=type->u.structure;
            while(params!=NULL){
                if(!strcmp(params->name, entry->name)){
                    printf("Error type 15 at Line %d: Redefined field %s.\n",varDec->lineNo,entry->name);
                    break;
                }
                params=params->tail;
            }
            //确保域名不重复后才插入
            if(params==NULL){
                if(find(entry->name, 2)!=NULL){
                    printf("Error type 3 at Line %d: Redefined variable %s.\n", varDec->lineNo, entry->name);
                }else{
                    insert(entry);
                    FieldList field=createFieldFromEntry(entry);
                    field->tail=type->u.structure;
                    type->u.structure=field;
                }
            }
            //printf("field name %s\n",type->u.structure->name);
            //printf("head %p\n",type->u.structure);
            //printf("next field %p\n",type->u.structure->tail);
            if(decList->childrenCount==3){
                decList=decList->children[2];
            }else{
                break;
            }
        }
        defList=defList->children[1];
    }
    SymbolTableEntry* entry_struct=(SymbolTableEntry*)malloc(sizeof(struct SymbolTableEntry));
    entry_struct->type=type;
    if(node->children[1]==NULL){
        //匿名结构体类型的名字怎么取？
        entry_struct->name=NULL;
    }else{
        entry_struct->name=node->children[1]->children[0]->value;
    }
    if(find(entry_struct->name, 2)){
        printf("Error type 16 at Line %d: Duplicated struct name %s.\n",node->children[1]->children[0]->lineNo,entry_struct->name);
    }else{
        insert(entry_struct);
    }
    return type;
}

//VarDec → ID
// | VarDec LB INT RB
SymbolTableEntry* VarDec(TreeNode* node, Type type){
    //维数
    int num=0;
    TreeNode* temp=node;
    //printf("debug\n");
    while(strcmp(temp->children[0]->name, "ID")){
        num++;
        temp=temp->children[0];
    }
    //printf("hhh%d\n",num);

    if(!strcmp(node->children[0]->name, "ID")){
        //num=0
        SymbolTableEntry* entry=(SymbolTableEntry*)malloc(sizeof(struct SymbolTableEntry));
        entry->name=node->children[0]->value;
        entry->type=type;
        return entry;
    }else{
        //num>0
        SymbolTableEntry* entry=(SymbolTableEntry*)malloc(sizeof(struct SymbolTableEntry));
        entry->name=temp->children[0]->value;
        Type lastType=type;
        temp=node;
        for(int i=0;i<num;i++){
            Type typei=(Type)malloc(sizeof(struct Type_));
            typei->kind=ARRAY;
            typei->u.array.elem=lastType;
            typei->u.array.size=atoi(temp->children[2]->value);
            lastType=typei;
            temp=temp->children[0];
            entry->type=typei;
        }
        return entry;
    }
}

//FunDec → ID LP VarList RP
// | ID LP RP
//has parameter or not
//VarList → ParamDec COMMA VarList
// | ParamDec
//ParamDec → Specifier VarDec
void FunDec(TreeNode* node, Type type){
    SymbolTableEntry* entry=(SymbolTableEntry*)malloc(sizeof(struct SymbolTableEntry));
    entry->name=node->children[0]->value;
    if(!strcmp(node->children[2]->name, "VarList")){
        //has params
        Type type0=(Type)malloc(sizeof(struct Type_));
        type0->kind=FUNC;
        type0->u.func.paramNum=0;
        type0->u.func.params=NULL;
        type0->u.func.returnType=type;
        TreeNode* varList=node->children[2];
        while(1){
            TreeNode* paramDec=varList->children[0];
            Type typep=Specifier(paramDec->children[0]);
            SymbolTableEntry* entryp=VarDec(paramDec->children[1], typep);
            int lineNo=paramDec->children[1]->lineNo;
            if(find(entryp->name, 2)!=NULL){
                printf("Error type 3 at Line %d: Redefined variable %s.\n", lineNo, entryp->name);
            }else{
                insert(entryp);
            }
            FieldList param=createFieldFromEntry(entryp);
            type0->u.func.paramNum++;
            param->tail=type0->u.func.params;
            type0->u.func.params=param;
            //printf("param name %s\n",type0->u.func.params->name);
            //printf("head %p\n",type0->u.func.params);
            //printf("next param %p\n",type0->u.func.params->tail);
            if(varList->childrenCount==3){
                varList=varList->children[2];
            }else{
                break;
            }
        }
        entry->type=type0;
    }else if(!strcmp(node->children[2]->name, "RP")){
        //has no params
        Type type0=(Type)malloc(sizeof(struct Type_));
        type0->kind=FUNC;
        type0->u.func.paramNum=0;
        type0->u.func.params=NULL;
        type0->u.func.returnType=type;
        entry->type=type0;
    }
    //printf("debug1\n");
    if(find(entry->name, 1)!=NULL){
        printf("Error type 4 at Line %d: Redefined function %s.\n", node->lineNo, node->children[0]->value);
    }else{
        insert(entry);
    }
    //printf("debug2\n");
}

//CompSt → LC DefList StmtList RC
//StmtList → Stmt StmtList
// | 
void CompSt(TreeNode* node, Type type){
    if(node==NULL) return;
    DefList(node->children[1]);
    TreeNode* stmtList=node->children[2];
    while(stmtList!=NULL){
        Stmt(stmtList->children[0], type);
        stmtList=stmtList->children[1];
    }
}

//Stmt → Exp SEMI
// | CompSt
// | RETURN Exp SEMI
// | IF LP Exp RP Stmt
// | IF LP Exp RP Stmt ELSE Stmt
// | WHILE LP Exp RP Stmt
void Stmt(TreeNode* node, Type type){
    if(!strcmp(node->children[0]->name, "Exp")){
        Exp(node->children[0]);
    }else if(!strcmp(node->children[0]->name, "CompSt")){
        CompSt(node->children[0], type);
    }else if(!strcmp(node->children[0]->name, "RETURN")){
        Type type_return=Exp(node->children[1]);
        //printf("debug1\n");
        if(typeMatch(type,type_return)==0){
            printf("Error type 8 at Line %d: Type mismatched for return.\n", node->lineNo);
        }
    }else if(!strcmp(node->children[0]->name, "IF")){
        if(node->childrenCount==5){
            Exp(node->children[2]);
            Stmt(node->children[4], type);
        }else{
            Exp(node->children[2]);
            Stmt(node->children[4], type);
            Stmt(node->children[6], type);
        }
    }else if(!strcmp(node->children[0]->name, "WHILE")){
        Exp(node->children[2]);
        Stmt(node->children[4], type);
    }
}

//DefList → Def DefList
// | 
void DefList(TreeNode* node){
    if(node!=NULL){
        Def(node->children[0]);
        DefList(node->children[1]);
    }
}

//Def → Specifier DecList SEMI
void Def(TreeNode* node){
    Type type=Specifier(node->children[0]);
    DecList(node->children[1], type);
}

//DecList → Dec
// | Dec COMMA DecList
void DecList(TreeNode* node, Type type){
    if(node!=NULL){
        Dec(node->children[0], type);
        if(node->childrenCount==3){
            DecList(node->children[2], type);
        }
    }
}

//Dec → VarDec
// | VarDec ASSIGNOP Exp
void Dec(TreeNode* node, Type type){
    //printf("debug\n");
    if(node==NULL) return;
    SymbolTableEntry* entry=VarDec(node->children[0], type);
    if(node->childrenCount==3){
        Type type_right=Exp(node->children[2]);
        //printf("debug\n");
        if(typeMatch(entry->type, type_right)==0){
            if(type_right!=NULL){
                printf("Error type 5 at Line %d: Type mismatched for assignment.\n",node->lineNo);
            }
        }
    }
    if(find(entry->name, 2)!=NULL){
        printf("Error type 3 at Line %d: Redefined variable %s.\n", node->lineNo, entry->name);
    }else{
        insert(entry);
    }
}

//Exp → Exp ASSIGNOP Exp
// | Exp AND Exp
// | Exp OR Exp
// | Exp RELOP Exp
// | Exp PLUS Exp
// | Exp MINUS Exp
// | Exp STAR Exp
// | Exp DIV Exp
// | LP Exp RP
// | MINUS Exp
// | NOT Exp
// | ID LP Args RP
// | ID LP RP
// | Exp LB Exp RB
// | Exp DOT ID
// | ID
// | INT
// | FLOAT
//Args → Exp COMMA Args
// | Exp
Type Exp(TreeNode* node){
    //printf("debug\n");
    if(node==NULL) return NULL;
    if(node->childrenCount>1){
        if(!strcmp(node->children[1]->name, "ASSIGNOP")){
            //ID
            //Exp LB Exp RB
            //Exp DOT ID
            TreeNode* exp_left=node->children[0];
            TreeNode* exp_right=node->children[2];
            if(exp_left->childrenCount==1 && !strcmp(exp_left->children[0]->name, "ID")
                || exp_left->childrenCount==4 && !strcmp(exp_left->children[2]->name, "Exp")
                || exp_left->childrenCount==3 && !strcmp(exp_left->children[1]->name, "DOT")){
                    ;//do nothing
            }else{
                printf("Error type 6 at Line %d: The left-hand side of an assignment can't be a right variable.\n", exp_left->lineNo);
            }
            Type t_left=Exp(exp_left);
            Type t_right=Exp(exp_right);
            //printf("debug1\n");
            if(typeMatch(t_left, t_right)==0){
                //printf("debug\n");
                //这里会造成重复报错?done
                if(t_left!=NULL&&t_right!=NULL){
                    printf("Error type 5 at Line %d: Type mismatched for assignment\n",node->lineNo);
                }
                return NULL;
            }
            return t_left;
        }else if(!strcmp(node->children[1]->name, "AND") || !strcmp(node->children[1]->name, "OR") || !strcmp(node->children[1]->name, "RELOP")){
            //操作数类型不匹配或操作数类型与操作符不匹配(仅有 int 型变量才能进行逻辑运算或者作为 if 和 while 语句的条件)
            TreeNode * exp_left = node->children[0];
            TreeNode * exp_right = node->children[2];
            Type t_left = Exp(exp_left);
            Type t_right = Exp(exp_right);
            if(t_left==NULL || t_right==NULL){
                return NULL;
            }
            //printf("debug1\n");
            if(t_left->kind==BASIC&&t_left->u.basic==TYPE_INT&&t_right->kind==BASIC&&t_right->u.basic==TYPE_INT){
                return t_left;
            }else{
                printf("Error type 7 at Line %d: Type mismatched for operands\n",node->lineNo);
                return NULL;
            }
        }else if(!strcmp(node->children[1]->name, "PLUS") || !strcmp(node->children[1]->name, "MINUS") || !strcmp(node->children[1]->name, "STAR") || !strcmp(node->children[1]->name, "DIV")){
            //操作数类型不匹配或操作数类型与操作符不匹配(例如整型变量与数组变量相加减，或数组（或结构体）变量与数组（或结构体）变量相加减)(仅有 int 型和 float 型变量才能参与算术运算。)
            TreeNode * exp_left = node->children[0];
            TreeNode * exp_right = node->children[2];
            Type t_left = Exp(exp_left);
            Type t_right = Exp(exp_right);
            if(t_left->kind==BASIC && t_right->kind==BASIC && typeMatch(t_left, t_right)==1){
                return t_left;
            }else{
                printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->lineNo);
                return NULL;
            }
        }else if(!strcmp(node->children[0]->name, "LP") || !strcmp(node->children[0]->name, "MINUS")){
            TreeNode * exp0 = node->children[1];
            Type t0 = Exp(exp0);
            if(t0->kind==BASIC){
                return t0;
            }else{
                printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->lineNo);
                return NULL;   
            }
        }else if(!strcmp(node->children[0]->name, "NOT")){
            TreeNode * exp0 = node->children[1];
            Type t0 = Exp(exp0);
            if(t0->kind==BASIC&&t0->u.basic==TYPE_INT){
                return t0;
            }else{
                printf("Error type 7 at Line %d: Type mismatched for operands\n",node->lineNo);
                return NULL;
            }
        }else if(!strcmp(node->children[0]->name, "ID")){
            //处理函数调用
            SymbolTableEntry* entry1=find(node->children[0]->value, 1);
            SymbolTableEntry* entry2=find(node->children[0]->value, 2);
            if(entry1==NULL&&entry2==NULL){
                printf("Error type 2 at Line %d: Undefined function %s.\n", node->children[0]->lineNo, node->children[0]->value);
                return NULL;
            }else if(entry1==NULL&&entry2!=NULL){
                printf("Error type 11 at Line %d: %s is not a function.\n", node->children[0]->lineNo, entry2->name);
                return NULL;
            }
            //检查函数实参与形参的数量和类型是否匹配
            Type type_func=entry1->type;
            Type type_use = (Type)malloc(sizeof(struct Type_));
            type_use->kind = FUNC;
            type_use->u.func.paramNum =0;
            type_use->u.func.params = NULL;
            if(!strcmp(node->children[2]->name, "Args")){
                TreeNode* args=node->children[2];
                while(1){
                    Type type_param=Exp(args->children[0]);
                    FieldList params=(FieldList)malloc(sizeof(struct FieldList_));
                    //实参的名字不重要
                    params->name="temp";
                    params->type=type_param;
                    type_use->u.func.paramNum++;
                    params->tail=type_use->u.func.params;
                    type_use->u.func.params=params;
                    //printf("real param type %d\n",type_use->u.func.params->type->kind);
                    //printf("real param name %s\n",type_use->u.func.params->name);
                    //printf("real head %p\n",type_use->u.func.params);
                    //printf("real next param %p\n",type_use->u.func.params->tail);
                    if(args->childrenCount==3){
                        args=args->children[2];
                    }else{
                        break;
                    }
                }
            }
            type_use->u.func.returnType=type_func->u.func.returnType;
            //printf("real return type %p\n",type_use->u.func.returnType);
            //printf("formal return type %p\n",type_func->u.func.returnType);
            //此处判断参数数量是否为0
            if(typeMatch(type_func, type_use)==0){
                //printf("debug\n");
                if(!(type_use->u.func.paramNum==0&&type_func->u.func.paramNum==0)){
                    printf("Error type 9 at Line %d: Params wrong in function %s.\n", node->lineNo, node->children[0]->value);
                    return NULL;
                }
                return type_func->u.func.returnType;
            }else{
                return type_func->u.func.returnType;
            }
        }else if(!strcmp(node->children[1]->name, "LB")){
            //printf("debug\n");
            Type type0=Exp(node->children[0]);
            //undefined
            if(type0==NULL){
                return NULL;
            }else if(type0->kind!=ARRAY){
                printf("Error type 10 at Line %d: %s is not an array.\n", node->children[0]->lineNo, node->children[0]->children[0]->value);
                return NULL;
            }
            Type type1=Exp(node->children[2]);
            if(type1==NULL){
                return NULL;
            }else if(!(type1->kind==BASIC&&type1->u.basic==TYPE_INT)){
                printf("Error type 12 at Line %d: there is not an integer in [" "].\n", node->children[2]->lineNo);
                return NULL;
            }
            return type0->u.array.elem;
        }else if(!strcmp(node->children[1]->name, "DOT")){
            //printf("debug\n");
            TreeNode* exp=node->children[0];
            Type type0=Exp(exp);
            if(type0==NULL){
                return NULL;
            }else if(type0->kind!=STRUCTURE){
                printf("Error type 13 at Line %d: %s is not a struct.\n", node->children[0]->lineNo, exp->children[0]->value);
                return NULL;
            }
            FieldList field=type0->u.structure;
            while(field!=NULL){
                //printf("struct field name %s\n",field->name);
                //printf("head %p\n",field);
                //printf("field name %s\n",node->children[2]->value);
                //printf("next field %p\n",type->u.structure->tail);
                if(!strcmp(node->children[2]->value, field->name)){
                    return field->type;
                }
                field=field->tail;
            }
            printf("Error type 14 at Line %d: Non-existent field %s.\n", node->children[2]->lineNo, node->children[2]->value);
            return NULL;
        }else{
            return NULL;
        }
    }else{
        if(!strcmp(node->children[0]->name, "ID")){
            SymbolTableEntry* entry=find(node->children[0]->value, 2);
            if(entry==NULL){
                printf("Error type 1 at Line %d: Undefined variable %s.\n",node->lineNo, node->children[0]->value);
                return NULL;
            }
            return entry->type;
        }else if(!strcmp(node->children[0]->name, "INT")){
            Type type0=(Type)malloc(sizeof(struct Type_));
            type0->kind=BASIC;
            type0->u.basic=TYPE_INT;
            return type0;
        }else if(!strcmp(node->children[0]->name, "FLOAT")){
            Type type0=(Type)malloc(sizeof(struct Type_));
            type0->kind=BASIC;
            type0->u.basic=TYPE_FLOAT;
            return type0;
        }else{
            return NULL;
        }
    }
}