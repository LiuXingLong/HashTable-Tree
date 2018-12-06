#include "bst.h"

unsigned int bst::BKDRHash(string key)
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
BstNode * bst::get_min_node(BstNode *&bst_p)
{
    if(bst_p->lchild == nullptr){
        return bst_p;
    }else{
        return this->get_min_node(bst_p->lchild);
    }
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

bool bst::bst_set(string key, string value,unsigned int hash_key, BstNode *&bst_p,BstNode *parent)
{
    if(bst_p == nullptr){
        bst_p = new BstNode;
        bst_p->data = new BstData;
        bst_p->lchild = nullptr;
        bst_p->rchild = nullptr;
        bst_p->parent = parent;
        bst_p->data->next = nullptr;
        bst_p->data->key = key;
        bst_p->data->value = value;
        bst_p->hash_key = hash_key;
    }else{
        if(hash_key < bst_p->hash_key){
            this->bst_set(key,value,hash_key,bst_p->lchild,bst_p);
        }else if(hash_key > bst_p->hash_key){
            this->bst_set(key,value,hash_key,bst_p->rchild,bst_p);
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

bool bst::bst_del(string key, unsigned int hash_key, BstNode *&bst_p)
{
    if(bst_p == nullptr){
        return true;  /// 数据不存在
    }else if(hash_key < bst_p->hash_key){
        return this->bst_del(key,hash_key,bst_p->lchild);
    }else if(hash_key > bst_p->hash_key){
        return this->bst_del(key,hash_key,bst_p->rchild);
    }else if(hash_key == bst_p->hash_key){
        BstData *bst_data;
        bst_data = bst_p->data;
        if(bst_data->next == nullptr){
            if(bst_data->key != key){
                return true;
            }
            /*** 删除节点  ***/
            if(bst_p->lchild != nullptr && bst_p->rchild != nullptr){
                /// 删除含两个子节点的  父节点
                BstNode *bst_h;
                bst_h = bst_p;
                bst_p = this->get_min_node(bst_p->rchild);
                /// 处理删除节点，右子树中最小节点的孩子
                if(bst_p->rchild != nullptr){
                    if(bst_p->parent->hash_key > bst_p->hash_key){
                        bst_p->parent->lchild = bst_p->rchild;
                    }else if(bst_p->parent->hash_key < bst_p->hash_key){
                        bst_p->parent->rchild = bst_p->rchild;
                    }
                    bst_p->rchild->parent = bst_p->parent;
                }else{
                    if(bst_p->parent->hash_key > bst_p->hash_key){
                        bst_p->parent->lchild = nullptr;
                    }else if(bst_p->parent->hash_key < bst_p->hash_key){
                        bst_p->parent->rchild = nullptr;
                    }
                }
                /// 处理删除节点，左右孩子
                bst_p->lchild = bst_h->lchild;
                bst_h->lchild->parent = bst_p;
                bst_p->rchild = bst_h->rchild;
                if(bst_h->rchild != nullptr){
                    bst_h->rchild->parent = bst_p;
                }
                /// 处理删除节点，父亲节点
                if(bst_h->parent == nullptr){
                    bst_p->parent = nullptr;
                }else if(bst_h->parent->hash_key > bst_h->hash_key){
                    bst_h->parent->lchild = bst_p;
                    bst_p->parent = bst_h->parent;
                }else if(bst_h->parent->hash_key < bst_h->hash_key){
                    bst_h->parent->rchild = bst_p;
                    bst_p->parent = bst_h->parent;
                }
                delete bst_h;
                bst_h = nullptr;
            }else if(bst_p->lchild != nullptr){
                /// 删除含左子节点的  父节点
                if(bst_p->parent == nullptr){
                    bst_p = bst_p->lchild;
                    delete bst_p->parent;
                    bst_p->parent = nullptr;
                }else if(bst_p->parent->hash_key > bst_p->hash_key){
                    BstNode *bst_h;
                    bst_h = bst_p;
                    bst_h->parent->lchild = bst_h->lchild;
                    bst_h->lchild->parent = bst_h->parent;
                    delete bst_h;
                    bst_h = nullptr;
                }else if(bst_p->parent->hash_key < bst_p->hash_key){
                    BstNode *bst_h;
                    bst_h = bst_p;
                    bst_h->parent->rchild = bst_h->lchild;
                    bst_h->lchild->parent = bst_h->parent;
                    delete bst_h;
                    bst_h = nullptr;
                }
            }else if(bst_p->rchild != nullptr){
                /// 删除含右子节点的  父节点
                if(bst_p->parent == nullptr){
                    bst_p = bst_p->rchild;
                    delete bst_p->parent;
                    bst_p->parent = nullptr;
                }else if(bst_p->parent->hash_key > bst_p->hash_key){
                    BstNode *bst_h;
                    bst_h = bst_p;
                    bst_h->parent->lchild = bst_h->rchild;
                    bst_h->rchild->parent = bst_h->parent;
                    delete bst_h;
                    bst_h = nullptr;
                }else if(bst_p->parent->hash_key < bst_p->hash_key){
                    BstNode *bst_h;
                    bst_h = bst_p;
                    bst_h->parent->rchild = bst_h->rchild;
                    bst_h->rchild->parent = bst_h->parent;
                    delete bst_h;
                    bst_h = nullptr;
                }
            }else{
                /// 删除叶节点
                if(bst_p->parent == nullptr){
                    delete bst_p;
                    bst_p = nullptr;
                }else if(bst_p->parent->hash_key > bst_p->hash_key){
                    bst_p->parent->lchild = nullptr;
                    delete bst_p;
                    bst_p = nullptr;
                }else if(bst_p->parent->hash_key < bst_p->hash_key){
                    bst_p->parent->rchild = nullptr;
                    delete bst_p;
                    bst_p = nullptr;
                }
            }
        }else{
            /*** 删除节点数据链中的数据 ***/
            int i = 0;
            BstData *prev_data;
            prev_data = bst_p->data;
            while( bst_data->next != nullptr && key != bst_data->key ){
                bst_data = bst_data->next;
                if( i > 0 ){
                    prev_data = prev_data->next;
                }
                i++;
            }
            if( key == bst_data->key ){
                if( i == 0 ){
                    prev_data = bst_data->next;
                }else{
                    prev_data->next = bst_data->next;
                }
                delete bst_data;
                bst_data = nullptr;
            }
        }
        return true;
    }
}
