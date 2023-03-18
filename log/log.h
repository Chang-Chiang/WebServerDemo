/**
 * @file log.h
 * @author Chang Chiang (Chang_Chiang@outlook.com.com)
 * @brief 日志类定义, 懒汉单例模式
 * @version 0.1
 * @date 2023-03-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOG_H
#define LOG_H

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

#include <iostream>
#include <string>

#include "block_queue.h"

using namespace std;

class Log {
public:
    // C++11以后, 使用局部变量懒汉单例不用加锁
    // 公有静态方法获取实例
    static Log* get_instance() {
        static Log instance;
        return &instance;
    }

    // 异步写日志
    static void* flush_log_thread(void* args) {
        Log::get_instance()->async_write_log();
    }

    // 初始化日志类
    // 可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    // 异步需要设置阻塞队列的长度，同步不需要设置
    bool init(
        const char* file_name, int log_buf_size = 8192, int split_lines = 5000000,
        int max_queue_size = 0);

    // 向日志文件写具体内容
    void write_log(int level, const char* format, ...);

    // 强制刷新缓冲区
    void flush(void);

private:
    // 日志类构造函数, 单例模式,私有化
    Log();

    // 日志类析构函数
    virtual ~Log();

    // 异步写日志
    void* async_write_log() {
        string single_log;
        // 从阻塞队列中取出一个日志string，写入文件
        while (m_log_queue->pop(single_log)) {
            m_mutex.lock();                  // 加锁
            fputs(single_log.c_str(), m_fp); // 写日志文件
            m_mutex.unlock();                // 解锁
        }
    }

private:
    char                 dir_name[128];  // 路径名
    char                 log_name[128];  // log文件名
    int                  m_split_lines;  // 日志最大行数
    int                  m_log_buf_size; // 日志缓冲区大小
    long long            m_count;        // 日志行数记录
    int                  m_today;        // 因为按天分类,记录当前时间是那一天
    FILE*                m_fp;           // 打开log的文件指针
    char*                m_buf;          // 日志缓冲区
    block_queue<string>* m_log_queue;    // 阻塞队列
    bool                 m_is_async;     // 是否同步标志位
    locker               m_mutex;        // 互斥锁
};

// 这四个宏定义在其他文件中使用，主要用于不同类型的日志输出
// __VA_ARGS__是一个可变参数的宏
// __VA_ARGS__宏前面加上##的作用在于，当可变参数的个数为0时，会把前面多余的","去掉，否则会编译出错。

#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__)

#endif
