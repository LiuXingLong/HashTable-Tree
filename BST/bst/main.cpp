#include <time.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include "bst.h"
#pragma comment(linker, "/STACK:102400000,102400000")

int main()
{
    bst BstMap;
    int number;
    ifstream infile;
    ofstream outfile;
    string clear_cmd,cmd,key,value,filename;
    BstNode *parent = nullptr, *bst_p = nullptr;
    clock_t s_time, e_time;
    s_time = clock();
    cout << "Input Data loading number : 1 to 10 ( input 0 not loading data )" << endl;
    cin >> number;
    for(int i = 1; i <= number ; i++){
        filename = "data\\set_" + std::to_string(i) + ".in";
        cout << filename << endl;
        infile.open(filename.c_str(),ios::in);
        while( !infile.eof() ){
            infile >> key;infile >> value;
            BstMap.bst_set(key,value,BstMap.BKDRHash(key),bst_p,parent);
        }
        infile.close();
        e_time = clock();
        cout<< "Data loading Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
    }
    /// 加载更新数据
    infile.open("data\\set_update.in",ios::in);
    while( !infile.eof() ){
        infile >> key;infile >> value;
        BstMap.bst_set(key,value,BstMap.BKDRHash(key),bst_p,parent);
    }
    infile.close();
    e_time = clock();
    cout << "Data loading Success" << endl;
    cout << "Data loading Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
    cout << "sample_get:" << "get"<< " " << "key" << endl;
    cout << "sample_set:" << "set"<< " " << "key" << " " << "value" << endl;
    outfile.open("data\\set_update.in",ios::app);
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
            s_time = clock();
            cout << BstMap.bst_get(key,BstMap.BKDRHash(key),bst_p) << endl;
            e_time = clock();
            cout << "Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
        }else if(cmd == "del"){
            bool del_status;
            s_time = clock();
            del_status = BstMap.bst_del(key,BstMap.BKDRHash(key),bst_p);
            cout << del_status << endl;
            e_time = clock();
            cout << "Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
        }else{
            bool set_status;
            s_time = clock();
            set_status = BstMap.bst_set(key,value,BstMap.BKDRHash(key),bst_p,parent);
            cout << set_status << endl;
            outfile << key << " " << value <<endl;
            e_time = clock();
            cout << "Time:" << (double)(e_time-s_time)/CLOCKS_PER_SEC << "s" << endl;
        }
    }
    outfile.close();
    return 0;
}
