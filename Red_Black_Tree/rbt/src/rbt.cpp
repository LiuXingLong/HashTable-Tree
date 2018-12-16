#include "rbt.h"

unsigned int rbt::BKDRHash(string key)
{
    unsigned int seed = 31; /// 也可以乘以31、131、1313、13131、131313..
    unsigned int hash = 0;
    unsigned int len = key.length();
    for (int i = 0; i < len; i++)
    {
         hash = hash*seed + key[i];
    }
    return (hash & 0x7FFFFFFF);
}

/// 左旋
bool rbt::rbt_left_rotate(RbtNode *&rbt_p)
{
    RbtNode *rp = rbt_p->rchild;
    rbt_p->rchild = rp->lchild;
    rp->lchild = rbt_p;
    if(rbt_p->rchild != nullptr){
        rbt_p->rchild->parent = rbt_p;
    }
    rp->parent = rbt_p->parent;
    if(rbt_p->parent == nullptr){
        rbt_p->parent = rp;
        rbt_p = rp; /// root = rp;
    }else{
        if(rbt_p->parent->hash_key > rbt_p->hash_key){
            rbt_p->parent->lchild = rp;
        }else if(rbt_p->parent->hash_key < rbt_p->hash_key){
            rbt_p->parent->rchild = rp;
        }
        rbt_p->parent = rp;
    }
    return true;
}

/// 右旋
bool rbt::rbt_right_rotate(RbtNode *&rbt_p)
{
    RbtNode *lp = rbt_p->lchild;
    rbt_p->lchild = lp->rchild;
    lp->rchild = rbt_p;
    if(rbt_p->lchild != nullptr){
        rbt_p->lchild->parent = rbt_p;
    }
    lp->parent = rbt_p->parent;
    if(rbt_p->parent == nullptr){
        rbt_p->parent = lp;
        rbt_p = lp;  /// root = lp;
    }else{
        if(rbt_p->parent->hash_key > rbt_p->hash_key){
            rbt_p->parent->lchild = lp;
        }else if(rbt_p->parent->hash_key < rbt_p->hash_key){
            rbt_p->parent->rchild = lp;
        }
        rbt_p->parent = lp;
    }
    return true;
}

/// 插入调整
bool rbt::rbt_set_fix_up(RbtNode *rbt_p)
{
    while(rbt_p->parent != nullptr && rbt_p->parent->parent != nullptr && rbt_p->parent->color == RED){
        if(rbt_p->parent->parent->hash_key > rbt_p->parent->hash_key){
            /// 父亲为左孩子
            RbtNode *ppr = rbt_p->parent->parent->rchild;
            if(ppr != nullptr && ppr->color == RED){
                rbt_p->parent->color = BLACK;
                ppr->color = BLACK;
                rbt_p->parent->parent->color = RED;
                rbt_p = rbt_p->parent->parent;
            }else{
                if(rbt_p->parent->hash_key < rbt_p->hash_key){
                    /// 插入节点为右孩子
                    rbt_p = rbt_p->parent;
                    this->rbt_left_rotate(rbt_p);
                }
                rbt_p->parent->color = BLACK;
                rbt_p->parent->parent->color = RED;
                this->rbt_right_rotate(rbt_p->parent->parent);
            }
        }else if(rbt_p->parent->parent->hash_key < rbt_p->parent->hash_key){
            /// 父亲为右孩子
            RbtNode *ppl = rbt_p->parent->parent->lchild;
            if(ppl != nullptr && ppl->color == RED){
                rbt_p->parent->color = BLACK;
                ppl->color = BLACK;
                rbt_p->parent->parent->color = RED;
                rbt_p = rbt_p->parent->parent;
            }else{
                if(rbt_p->parent->hash_key > rbt_p->hash_key){
                    /// 插入节点为左孩子
                    rbt_p = rbt_p->parent;
                    this->rbt_right_rotate(rbt_p);
                }
                rbt_p->parent->color = BLACK;
                rbt_p->parent->parent->color = RED;
                this->rbt_left_rotate(rbt_p->parent->parent);
            }
        }
    }
    if(rbt_p->parent == nullptr){
        rbt_p->color = BLACK;
    }else if(rbt_p->parent->parent == nullptr){
        rbt_p->parent->color = BLACK;
    }
    return true;
}

/// 删除调整
bool rbt::rbt_del_fix_up(RbtNode *rbt_p)
{


}

string rbt::rbt_get(string key, unsigned int hash_key,RbtNode *rbt_p)
{
    if(rbt_p == nullptr){
        return "null";
    }else if(hash_key < rbt_p->hash_key){
        return this->rbt_get(key,hash_key,rbt_p->lchild);
    }else if(hash_key > rbt_p->hash_key){
        return this->rbt_get(key,hash_key,rbt_p->rchild);
    }else if(hash_key == rbt_p->hash_key){
        RbtData *rbt_data;
        rbt_data = rbt_p->data;
        while( rbt_data->next != nullptr && key != rbt_data->key ){
            rbt_data = rbt_data->next;
        }
        if( key == rbt_data->key ){
            return rbt_data->value;
            /*
            if(rbt_p->parent != nullptr){
                return rbt_data->value + "|" + rbt_p->parent->data->value;
            }else{
                return rbt_data->value + "|root";
            }*/
        }else{
            return "null";
        }
    }
}




bool rbt::rbt_set(string key, string value,unsigned int hash_key, RbtNode *&rbt_p,RbtNode *parent)
{
    if(rbt_p == nullptr){
        rbt_p = new RbtNode;
        rbt_p->data = new RbtData;
        rbt_p->lchild = nullptr;
        rbt_p->rchild = nullptr;
        rbt_p->color = RED;
        rbt_p->parent = parent;
        rbt_p->data->next = nullptr;
        rbt_p->data->key = key;
        rbt_p->data->value = value;
        rbt_p->hash_key = hash_key;
        /// 对插入节点进行调整
        this->rbt_set_fix_up(rbt_p);
    }else{
        if(hash_key < rbt_p->hash_key){
            this->rbt_set(key,value,hash_key,rbt_p->lchild,rbt_p);
        }else if(hash_key > rbt_p->hash_key){
            this->rbt_set(key,value,hash_key,rbt_p->rchild,rbt_p);
        }else if(hash_key == rbt_p->hash_key){
            RbtData *rbt_data;
            rbt_data = rbt_p->data;
            while( rbt_data->next != nullptr && key != rbt_data->key ){
                rbt_data = rbt_data->next;
            }
            if( key == rbt_data->key ){
                rbt_data->value = value;
            }else{
                 rbt_data->next = new RbtData;
                 rbt_data->next->key = key;
                 rbt_data->next->value = value;
                 rbt_data->next->next = nullptr;
            }
        }
    }
    return true;
}

bool rbt::rbt_del(string key, unsigned int hash_key, RbtNode *&rbt_p)
{



}



