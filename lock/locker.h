/**
 * @file locker.h
 * @author Chang Chiang (Chang_Chiang@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <semaphore.h>

#include <exception>

// 信号量
class sem {
public:
    // 构造函数, 初始化信号量
    sem() {
        if (sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }

    // 构造函数, 指定资源数初始化信号量
    sem(int num) {
        if (sem_init(&m_sem, 0, num) != 0) {
            throw std::exception();
        }
    }

    // 析构函数, 释放信号量资源
    ~sem() { sem_destroy(&m_sem); }

    // 调用该函数会将sem中的资源数-1。
    // 若sem中的资源数 > 0, 线程不会阻塞。
    // 若sem中的资源数减为 0, 资源被耗尽, 线程被阻塞。
    bool wait() { return sem_wait(&m_sem) == 0; }

    // 调用该函数会将 sem 中的资源数 +1。
    // 若有线程调用 wait() 被阻塞, 这时这些线程会解除阻塞,
    // 获取到资源之后继续向下运行。
    bool post() { return sem_post(&m_sem) == 0; }

private:
    sem_t m_sem;
};

// 互斥锁
class locker {
public:
    // 构造函数, 初始化互斥锁
    locker() {
        if (pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    // 析构函数, 释放互斥锁资源
    ~locker() { pthread_mutex_destroy(&m_mutex); }

    // 加锁
    bool lock() { return pthread_mutex_lock(&m_mutex) == 0; }

    // 解锁
    bool unlock() { return pthread_mutex_unlock(&m_mutex) == 0; }

    // 获取互斥锁地址
    pthread_mutex_t* get() { return &m_mutex; }

private:
    pthread_mutex_t m_mutex; // 互斥锁
};

// 条件变量
class cond {
public:
    // 构造函数, 初始化条件变量
    cond() {
        if (pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        }
    }

    // 析构函数, 释放条件变量资源
    ~cond() { pthread_cond_destroy(&m_cond); }

    // 线程阻塞函数，先把调用该函数的线程放入条件变量的请求队列。
    // 如果线程已经对互斥锁上锁，那么会将这把锁打开，这样做是为了避免死锁。
    // 在线程解除阻塞时，函数内部会帮助这个线程再次将锁锁上，继续向下访问临界区。
    bool wait(pthread_mutex_t* m_mutex) {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }

    // 非死等实现
    bool timewait(pthread_mutex_t* m_mutex, struct timespec t) {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }

    // 唤醒阻塞在条件变量上的线程
    bool signal() { return pthread_cond_signal(&m_cond) == 0; }

    // 广播方式唤醒所有阻塞在条件变量上的线程
    bool broadcast() { return pthread_cond_broadcast(&m_cond) == 0; }

private:
    pthread_cond_t m_cond; // 条件变量
};
#endif
