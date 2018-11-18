#include <stdio.h>
#include <iostream>
#include "bst.h"
#pragma comment(linker, "/STACK:102400000,102400000")

int main()
{
    bst BstMap;
    string clear_cmd,cmd,key,value;
    BstNode *bst_p = nullptr;
    while(cin >> cmd){
        /// 读取数据
        if(cmd != "get" && cmd != "set" && cmd != "del"){
             cout << "input Error" << endl;
             getline(cin,clear_cmd);
             cin.clear();
             continue;
        }
        if(cmd == "set"){
            cin >> key >> value;
        }else{
            cin >> key;
        }
        getline(cin,clear_cmd);
        clear_cmd.erase(0,clear_cmd.find_first_not_of(" "));
        if(clear_cmd.length()){
            cout << "input Error" << endl;
            cin.clear();
            continue;
        };
        /// 业务处理
        if(cmd == "get"){
            cout << BstMap.bst_get(key,BstMap.BKDRHash(key),bst_p) << endl;
        }else if(cmd == "del"){
            cout << cmd << " " << key << " Hash_Key: "<< BstMap.BKDRHash(key) << endl;
        }else{
            BstMap.bst_set(key,value,BstMap.BKDRHash(key),bst_p);
        }
    }
    return 0;
}
