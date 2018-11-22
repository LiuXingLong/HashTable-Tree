#ifndef BST_H
#define BST_H
#include<stdlib.h>
#include <string>
#include <iostream>

using namespace std;
typedef struct BstData
{
    string key;
    string value;
    BstData *next;
}BstData;

typedef struct BstNode
{
    unsigned int hash_key; /// BKDRHash ох
    BstData *data;
    BstNode *lchild,*rchild;
} BstNode;

class bst
{
    public:
        string bst_get(string key ,unsigned int hash_key, BstNode *node);
        bool bst_set(string key, string value,unsigned int hash_key, BstNode *&node);
        bool bst_del(string key, unsigned int hash_key, BstNode *node);
        unsigned int BKDRHash(string key);
};

#endif // BST_H
