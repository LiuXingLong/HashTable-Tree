#ifndef RBT_H
#define RBT_H
#include<stdlib.h>
#include <string>
#include <iostream>

using namespace std;
typedef struct RbtData
{
    string key;
    string value;
    RbtData *next;
}RbtData;

enum RrtColor{RED, BLACK};

typedef struct RbtNode
{
    unsigned int hash_key; /// BKDRHash
    RrtColor color;
    RbtData *data;
    RbtNode *parent,*lchild,*rchild;
}RbtNode;

class rbt
{
    public:
        string rbt_get(string key ,unsigned int hash_key, RbtNode *node);
        bool rbt_set(string key, string value,unsigned int hash_key, RbtNode *&node, RbtNode *parent);
        bool rbt_del(string key, unsigned int hash_key, RbtNode *&node);
        unsigned int BKDRHash(string key);
    private:
        bool rbt_left_rotate(RbtNode *&node);
        bool rbt_right_rotate(RbtNode *&node);
        bool rbt_set_fix_up(RbtNode *node);
        bool rbt_del_fix_up(RbtNode *node);
};


#endif // RBT_H
