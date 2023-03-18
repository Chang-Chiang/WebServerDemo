/**
 * @file lst_timer.h
 * @author Chang Chiang (Chang_Chiang@outlook.com.com)
 * @brief 升序定时器链表
 * 《Linux 高性能服务器编程》 第 11 章 定时器, 代码清单 11-2
 * @version 0.1
 * @date 2023-03-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>

#include "../log/log.h"

// 声明定时器类
class util_timer;

// 用户数据结构
struct client_data {
    sockaddr_in address; // 客户端 socket 地址
    int         sockfd;  // socket 文件描述符
    util_timer* timer;   // 定时器
};

// 定时器类定义, 双向链表节点定义
class util_timer {
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;                 // 超时时间
    void (*cb_func)(client_data*); // 回调函数
    client_data* user_data;        // 用户数据
    util_timer*  prev;             // 指向前一个定时器
    util_timer*  next;             // 指向下一个定时器
};

// 定时器链表, 升序、带头尾节点的双向链表
class sort_timer_lst {
public:
    sort_timer_lst() : head(NULL), tail(NULL) {}

    ~sort_timer_lst() {
        util_timer* tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    void add_timer(util_timer* timer) {

        // 新增节点为空
        if (!timer) {
            return;
        }

        // 往空链表中添加节点
        if (!head) {
            head = tail = timer;
            return;
        }

        // 新增定时器超时时间小于所有定时器超时时间
        // 定时器插入链表头部
        if (timer->expire < head->expire) {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer, head);
    }

    // 定时器发生变化时
    // 调整对应定时器在链表中的位置
    void adjust_timer(util_timer* timer) {

        if (!timer) {
            return;
        }

        util_timer* tmp = timer->next;

        // 定时器为尾节点或定时器超时时间仍小于后一定时器
        // 不用调整
        if (!tmp || (timer->expire < tmp->expire)) {
            return;
        }

        // 定时器为头节点
        // 从链表取出并重新插入链表
        if (timer == head) {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, head);
        }

        // 定时器节点向后移
        else {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer, timer->next);
        }
    }

    // 定时器从链表中删除
    void del_timer(util_timer* timer) {
        if (!timer) {
            return;
        }
        // 当前定时器为链表中唯一节点
        if ((timer == head) && (timer == tail)) {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }

        if (timer == head) {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }

        if (timer == tail) {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }

        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    // SIGALRM 被触发在其信号处理函数调用 tick()
    void tick() {
        if (!head) {
            return;
        }
        // printf( "timer tick\n" );
        LOG_INFO("%s", "timer tick");
        Log::get_instance()->flush();

        time_t      cur = time(NULL); // 获取系统当前时间
        util_timer* tmp = head;
        while (tmp) {

            // 定时器还未超时
            if (cur < tmp->expire) {
                break;
            }

            // 执行定时任务
            tmp->cb_func(tmp->user_data);

            // 执行定时任务后的定时器删除
            // 重置链表头节点
            head = tmp->next;
            if (head) {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    // 定时器添加到节点 lst_head 之后的链表部分
    void add_timer(util_timer* timer, util_timer* lst_head) {
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;
        while (tmp) {
            if (timer->expire < tmp->expire) {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        if (!tmp) {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }

private:
    util_timer* head;
    util_timer* tail;
};

#endif
