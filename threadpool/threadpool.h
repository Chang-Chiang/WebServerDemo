/**
 * @file threadpool.h
 * @author Chang Chiang (Chang_Chiang@outlook.com)
 * @brief 半同步/半反应堆线程池实现
 * @version 0.1
 * @date 2023-03-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

#include <cstdio>
#include <exception>
#include <list>

#include "../CGImysql/sql_connection_pool.h"
#include "../lock/locker.h"

template <typename T>
class threadpool {
public:
    // 构造函数
    // *thread_number是线程池中线程的数量
    // max_requests是请求队列中最多允许的等待处理的请求的数量
    threadpool(connection_pool* connPool, int thread_number = 8, int max_request = 10000);

    // 析构函数
    ~threadpool();

    // 向任务队列插入任务
    bool append(T* request);

private:
    // 工作线程运行的函数
    // 它不断从工作队列中取出任务并执行之
    static void* worker(void* arg);

    void run();

private:
    int              m_thread_number; // 线程池中的线程数
    int              m_max_requests;  // 请求队列中允许的最大请求数
    pthread_t*       m_threads;       // 描述线程池的数组，其大小为 m_thread_number
    std::list<T*>    m_workqueue;     // 双向链表实现请求队列
    locker           m_queuelocker;   // 保护请求队列的互斥锁
    sem              m_queuestat;     // 是否有任务需要处理的信号量
    bool             m_stop;          // 是否结束线程
    connection_pool* m_connPool;      // 数据库连接池指针
};

// 构造函数
template <typename T>
threadpool<T>::threadpool(connection_pool* connPool, int thread_number, int max_requests)
    : m_thread_number(thread_number)
    , m_max_requests(max_requests)
    , m_stop(false)
    , m_threads(NULL)
    , m_connPool(connPool) {

    if (thread_number <= 0 || max_requests <= 0) {
        throw std::exception();
    }

    // 动态分配线程池资源
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }

    // 循环创建线程, 同时设置其回调函数
    for (int i = 0; i < m_thread_number; ++i) {

        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }

        // 线程分离, 不用单独对工作线程回收
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

// 析构函数
template <typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

// 向任务队列插入任务
template <typename T>
bool threadpool<T>::append(T* request) {
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post(); // 信号量 + 1, 通知有任务要处理
    return true;
}

// 工作线程运行
template <typename T>
void* threadpool<T>::worker(void* arg) {
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait(); // 等待信号量
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front(); // 从任务队列取任务
        m_workqueue.pop_front();          // 将取出任务从任务队列中删除
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }

        // Proactor 模型
        // 主线程和内核负责处理读写数据、接受新连接等I/O操作
        // 工作线程仅负责业务逻辑
        connectionRAII mysqlcon(&request->mysql, m_connPool); // 获取连接
        request->process();                                   // 处理业务逻辑
    }
}

#endif
