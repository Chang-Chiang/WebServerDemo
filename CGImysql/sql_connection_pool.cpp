/**
 * @file sql_connection_pool.cpp
 * @author Chang Chiang (Chang_Chiang@outlook.com.com)
 * @brief
 * @version 0.1
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "sql_connection_pool.h"

#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
#include <string>

using namespace std;

connection_pool::connection_pool() {
    this->CurConn = 0;
    this->FreeConn = 0;
}

connection_pool* connection_pool::GetInstance() {
    static connection_pool connPool;
    return &connPool;
}

void connection_pool::init(
    string url, string User, string PassWord, string DBName, int Port, unsigned int MaxConn) {
    // 初始化数据库信息
    this->url = url;
    this->Port = Port;
    this->User = User;
    this->PassWord = PassWord;
    this->DatabaseName = DBName;

    lock.lock();

    // 创建 MaxConn 条数据库连接
    for (int i = 0; i < MaxConn; i++) {

        // 初始化连接环境
        MYSQL* con = NULL;
        con = mysql_init(con);
        if (con == NULL) {
            cout << "Error:" << mysql_error(con);
            exit(1);
        }

        // 连接 MySQL 服务器
        con = mysql_real_connect(
            con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);
        if (con == NULL) {
            cout << "Error: " << mysql_error(con);
            exit(1);
        }

        // 当前连接加入连接池
        connList.push_back(con);
        ++FreeConn; // 空闲连接数 +1
    }

    // 信号量设置为初始化空闲数, 即最大连接数
    reserve = sem(FreeConn);

    // 设置连接数
    this->MaxConn = FreeConn;

    lock.unlock();
}

MYSQL* connection_pool::GetConnection() {
    MYSQL* con = NULL;

    if (0 == connList.size()) {
        return NULL;
    }

    // 信号量 -1(原子), 为 0 时阻塞
    reserve.wait();

    lock.lock();

    // 连接池取出一个连接
    con = connList.front();
    connList.pop_front();

    --FreeConn; // 空闲连接数 -1
    ++CurConn;  // 当前已使用连接数 +1

    lock.unlock();
    return con;
}

bool connection_pool::ReleaseConnection(MYSQL* con) {
    if (NULL == con) {
        return false;
    }

    lock.lock();

    // 往连接池放回连接
    connList.push_back(con);
    ++FreeConn; // 空闲连接数 +1
    --CurConn;  // 当前已使用连接数 -1

    lock.unlock();

    // 信号量 +1(原子)
    reserve.post();
    return true;
}

void connection_pool::DestroyPool() {

    lock.lock();
    if (connList.size() > 0) {
        list<MYSQL*>::iterator it;
        // 迭代器遍历, 关闭所有数据库连接
        for (it = connList.begin(); it != connList.end(); ++it) {
            MYSQL* con = *it;
            mysql_close(con); // 关闭 MySQL 实例
        }
        CurConn = 0;      // 当前已使用连接数置 0
        FreeConn = 0;     // 空闲连接数置 0
        connList.clear(); // 清空数据库连接池

        lock.unlock();
    }

    lock.unlock();
}

int connection_pool::GetFreeConn() { return this->FreeConn; }

connection_pool::~connection_pool() { DestroyPool(); }

connectionRAII::connectionRAII(MYSQL** SQL, connection_pool* connPool) {
    // 从连接池获取一个连接
    *SQL = connPool->GetConnection();
    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() { poolRAII->ReleaseConnection(conRAII); }