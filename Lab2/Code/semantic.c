#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#define HASH_SIZE 101

typedef enum { VAR, FUNC } SymbolType;

typedef struct {
    char* type;
} VarInfo;

typedef struct {
    char* returnType;
    int paramCount;
    char** paramTypes;
} FuncInfo;

typedef struct SymbolTableEntry {
    char* name;
    SymbolType type;
    union {
        VarInfo var;
        FuncInfo func;
    } info;
    struct SymbolTableEntry* next;
} SymbolTableEntry;

//


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

void insert(char* name, SymbolTableEntry entry) {
    unsigned int index = hash_pjw(name) % HASH_SIZE;
    SymbolTableEntry* newEntry = malloc(sizeof(SymbolTableEntry));
    *newEntry = entry;
    newEntry->next = hashTable[index];
    hashTable[index] = newEntry;
}

SymbolTableEntry* find(char* name) {
    unsigned int index = hash_pjw(name) % HASH_SIZE;
    SymbolTableEntry* entry = hashTable[index];
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}


void semantic_analysis(TreeNode* root){

}