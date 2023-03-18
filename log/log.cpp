/**
 * @file log.cpp
 * @author Chang Chiang (Chang_Chiang@outlook.com.com)
 * @brief 日志类实现
 * @version 0.1
 * @date 2023-03-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "log.h"

#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
using namespace std;

Log::Log() {
    m_count = 0;        // 日志行数
    m_is_async = false; // 是否异步
}

Log::~Log() {
    if (m_fp != NULL) {
        fclose(m_fp);
    }
}

bool Log::init(const char* file_name, int log_buf_size, int split_lines, int max_queue_size) {

    // 如果设置了max_queue_size,则设置为异步
    if (max_queue_size >= 1) {

        m_is_async = true; // 异步写日志

        // 创建阻塞队列
        m_log_queue = new block_queue<string>(max_queue_size);

        // 创建线程异步写日志
        pthread_t tid;
        // flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    m_log_buf_size = log_buf_size;       // 日志缓冲区大小
    m_buf = new char[m_log_buf_size];    // 为缓冲区动态分配内存
    memset(m_buf, '\0', m_log_buf_size); // 初始化缓冲区
    m_split_lines = split_lines;         // 日志最大行数

    time_t     t = time(NULL);         // 获取当前时间戳
    struct tm* sys_tm = localtime(&t); // 通过时间戳获取系统时间
    struct tm  my_tm = *sys_tm;

    // "./a/b/c" -> "/c"
    // "a" -> NULL
    const char* p = strrchr(file_name, '/');
    char        log_full_name[256] = {0};

    if (p == NULL) {
        // 日志文件名
        // eg. 2023_03_07_ServerLog, file_name == "ServerLog"
        snprintf(
            log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
            my_tm.tm_mday, file_name);
    }
    else {
        // eg. 2023_03_07_ServerLog, file_name == "./ServerLog"
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(
            log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900,
            my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday; // 日志按天分类

    // 打开日志文件
    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL) {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);          // 获取当前时间戳
    time_t     t = now.tv_sec;         // 当前时间的秒数
    struct tm* sys_tm = localtime(&t); // 秒数格式化, 线程安全
    struct tm  my_tm = *sys_tm;

    // 日志分级
    char s[16] = {0};
    switch (level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }

    // 写入一个 log
    // m_count++, 日志行数
    // m_split_lines, 日志最大行数
    m_mutex.lock();
    m_count++; // 日志行数 +1

    // 日志时间不是当天 或 日志行数对最大行数求余 == 0
    // 创建新日志
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {

        char new_log[256] = {0}; // 新日志名
        fflush(m_fp);            // 刷新缓冲区
        fclose(m_fp);            // 关闭旧日志文件
        char tail[16] = {0};     // 日志名中的时间部分

        // yyyy_mm_dd
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        // 日志时间不是当天
        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday; // 日志时间修改为当天
            m_count = 0;             // 重置日志行数
        }

        // 日志行数为最大行倍数
        // 即每写满一页日志, 新建一页日志
        else {
            // 新建日志名:
            snprintf(
                new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock(); // 解锁

    // 将传入的 format 参数赋值给可变参数列表类型 valst, 便于格式化输出
    va_list valst;
    va_start(valst, format);

    string log_str;

    m_mutex.lock(); // 加锁
    // 写入的具体时间内容格式
    // eg. 2023-03-07 13:22:45.134070 [info]
    int n = snprintf(
        m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
        my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock(); // 解锁

    // 异步写日志
    if (m_is_async && !m_log_queue->full()) {
        // 阻塞队列未满则将日志信息加入阻塞队列
        m_log_queue->push(log_str);
    }
    // 阻塞队列满
    else {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void) {
    m_mutex.lock();
    // 强制将缓冲区内的数据写入指定的文件, 防止缓冲区被覆盖
    fflush(m_fp);
    m_mutex.unlock();
}
