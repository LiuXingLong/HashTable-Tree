#include "bst.h"

unsigned int bst::BKDRHash(string key)
{
    unsigned int seed = 31; /// Ҳ���Գ���31��131��1313��13131��131313..
    unsigned int hash = 0;
    unsigned int len = key.length();
    for (int i = 0; i < len; i++)
    {
         hash = hash*seed + key[i];
    }
    return (hash & 0x7FFFFFFF);
}

string bst::bst_get(string key, unsigned int hash_key,BstNode *bst_p)
{
    if(bst_p == nullptr){
        return "null";
    }else if(hash_key < bst_p->hash_key){
        return this->bst_get(key,hash_key,bst_p->lchild);
    }else if(hash_key > bst_p->hash_key){
        return this->bst_get(key,hash_key,bst_p->rchild);
    }else if(hash_key == bst_p->hash_key){
        BstData *bst_data;
        bst_data = bst_p->data;
        while( bst_data->next != nullptr && key != bst_data->key ){
            bst_data = bst_data->next;
        }
        if( key == bst_data->key ){
            return bst_data->value;
        }else{
            return "null";
        }
    }
}

bool bst::bst_set(string key, string value,unsigned int hash_key, BstNode *&bst_p)
{
    if(bst_p == nullptr){
        bst_p = new BstNode;
        bst_p->data = new BstData;
        bst_p->lchild = nullptr;
        bst_p->rchild = nullptr;
        bst_p->data->next = nullptr;
        bst_p->data->key = key;
        bst_p->data->value = value;
        bst_p->hash_key = hash_key;
    }else{
        if(hash_key < bst_p->hash_key){
            this->bst_set(key,value,hash_key,bst_p->lchild);
        }else if(hash_key > bst_p->hash_key){
            this->bst_set(key,value,hash_key,bst_p->rchild);
        }else if(hash_key == bst_p->hash_key){
            BstData *bst_data;
            bst_data = bst_p->data;
            while( bst_data->next != nullptr && key != bst_data->key ){
                bst_data = bst_data->next;
            }
            if( key == bst_data->key ){
                bst_data->value = value;
            }else{
                 bst_data->next = new BstData;
                 bst_data->next->key = key;
                 bst_data->next->value = value;
                 bst_data->next->next = nullptr;
            }
        }
    }
    return true;
}

bool bst::bst_del(string key, unsigned int hash_key , BstNode *bst_p)
{

}
