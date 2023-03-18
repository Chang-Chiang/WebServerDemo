/**
 * @file sql_connection_pool.h
 * @author Chang Chiang (Chang_Chiang@outlook.com.com)
 * @brief
 * @version 0.1
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <error.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <list>
#include <string>

#include "../lock/locker.h"

using namespace std;

class connection_pool {
public:
    // 获取数据库连接
    // 当有请求时, 从数据库连接池中返回一个可用连接, 更新使用和空闲连接数
    MYSQL* GetConnection();

    // 释放连接
    bool ReleaseConnection(MYSQL* conn);

    // 当前空闲的连接数
    int GetFreeConn();

    // 销毁数据库连接池
    void DestroyPool();

    // 单例模式
    static connection_pool* GetInstance();

    // 初始化
    void init(
        string url, string User, string PassWord, string DataBaseName, int Port,
        unsigned int MaxConn);

private:
    // 构造函数
    connection_pool();

    // 析构函数
    ~connection_pool();

private:
    unsigned int MaxConn;  // 最大连接数
    unsigned int CurConn;  // 当前已使用的连接数
    unsigned int FreeConn; // 当前空闲的连接数

private:
    locker       lock;     // 互斥锁
    list<MYSQL*> connList; // 双向循环链表实现连接池
    sem          reserve;  // 信号量

private:
    string url;          // 数据库主机地址
    string Port;         // 数据库端口号
    string User;         // 登陆数据库用户名
    string PassWord;     // 登陆数据库密码
    string DatabaseName; // 使用数据库名
};

// RAII 机制释放数据库连接
class connectionRAII {

public:
    // 构造函数
    // 连接池中成员为指针, 故这里使用二级指针
    connectionRAII(MYSQL** con, connection_pool* connPool);

    // 析构函数
    ~connectionRAII();

private:
    MYSQL*           conRAII;
    connection_pool* poolRAII;
};

#endif
