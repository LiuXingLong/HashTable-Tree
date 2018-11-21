#include <time.h>
#include <stdio.h>
#include <iostream>
#include "bst.h"
#pragma comment(linker, "/STACK:102400000,102400000")

int main()
{
    bst BstMap;
    int number;
    string clear_cmd,cmd,key,value,filename;
    BstNode *bst_p = nullptr;
    clock_t s_time, e_time;
    s_time = clock();
    cout << "Input Data loading number : 1 to 10" << endl;
    cin >> number;
    for(int i = 1; i <= number ; i++){
        filename = "data\\set_" + std::to_string(i) + ".in";
        cout << filename << endl;
        freopen(filename.c_str(),"r",stdin);
        while(cin >> cmd >> key >> value){
            BstMap.bst_set(key,value,BstMap.BKDRHash(key),bst_p);
        }
        e_time = clock();
        cout<<  "Data loading Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
        fclose(stdin);
        freopen("CON","r",stdin);
        cin.clear();
    }
    e_time = clock();
    cout << "Data loading Success" << endl;
    cout<<  "Data loading Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
    cout<<  "sample_get:" << "get"<< " " << "key" <<endl;
    cout<<  "sample_set:" << "set"<< " " << "key" << " " << "value" <<endl;

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
            bool set_status;
            set_status = BstMap.bst_set(key,value,BstMap.BKDRHash(key),bst_p);
            cout << set_status << endl;
        }
    }
    return 0;
}
