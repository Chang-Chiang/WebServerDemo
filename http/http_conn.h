/**
 * @file http_conn.h
 * @author Chang Chiang (Chang_Chiang@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-03-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../CGImysql/sql_connection_pool.h"
#include "../lock/locker.h"

// http 类
// 通过该类创建对象用于接收客户端 http 请求
// 并将所有数据读入对应 buffer
class http_conn {
public:
    static const int FILENAME_LEN = 200;       // 设置读取文件的名称 m_real_file 大小
    static const int READ_BUFFER_SIZE = 2048;  // 设置读缓冲区 m_read_buf 大小
    static const int WRITE_BUFFER_SIZE = 1024; // 设置写缓冲区 m_write_buf 大小

    // 报文的请求方法, 本项目只用到 GET 和 POST
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH };

    // 主状态机的可能状态
    // 正在分析请求行、正在分析头部字段、正在分析消息体
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };

    // 服务器处理 HTTP 请求的结果
    enum HTTP_CODE {
        NO_REQUEST,        // 请求不完整, 需要继续读取请求报文数据
        GET_REQUEST,       // 获得一个完整的 HTTP 请求
        BAD_REQUEST,       // HTTP 请求有语法错误
        NO_RESOURCE,       // 请求资源不存在
        FORBIDDEN_REQUEST, // 客户对资源没有足够的访问权限
        FILE_REQUEST,      // 请求资源可以正常访问
        INTERNAL_ERROR, // 服务器内部错误，该结果在主状态机逻辑 switch 的 default 下，一般不会触发
        CLOSED_CONNECTION // 客户端已经关闭连接
    };

    // 从状态机，用于分析出一行内容
    // 从状态机的 3 种可能状态
    // 读取到一个完整的行、行出错、行数据尚不完整
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn() {}
    ~http_conn() {}

public:
    // 初始化套接字地址, 函数内部会调用私有方法 init
    void init(int sockfd, const sockaddr_in& addr);

    // 关闭 http 连接
    void close_conn(bool real_close = true);

    // 报文解析
    void process();

    // 读取浏览器端发来的全部数据
    // 循环读取客户数据, 直到无数据可读或对方关闭连接
    bool read_once();

    // 响应报文写入函数
    bool write();

    sockaddr_in* get_address() { return &m_address; }

    // 同步线程初始化数据库读取表
    void initmysql_result(connection_pool* connPool);

private:
    // 初始化新接受的连接
    void init();

    // 从 m_read_buf 读取, 并处理请求报文
    HTTP_CODE process_read();

    // 向 m_write_buf 写入响应报文数据
    bool process_write(HTTP_CODE ret);

    // 主状态机解析报文中的请求行数据
    // 解析 http 请求行，获得请求方法，目标 url 及 http 版本号
    HTTP_CODE parse_request_line(char* text);

    // 主状态机解析报文中的请求头部数据
    HTTP_CODE parse_headers(char* text);

    // 主状态机解析报文中的请求内容
    HTTP_CODE parse_content(char* text);

    // 生成响应报文
    HTTP_CODE do_request();

    // get_line 用于将指针向后偏移, 指向未处理的字符
    // m_start_line 是已经解析的字符
    char* get_line() { return m_read_buf + m_start_line; };

    // 从状态机读取一行, 分析是请求报文的哪一部分
    // 返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
    LINE_STATUS parse_line();

    void unmap();

    /* 根据响应报文格式, 生成对应 8 个部分, 以下函数均由 do_request 调用 */

    bool add_response(const char* format, ...);

    // 添加文本content
    bool add_content(const char* content);

    // 添加状态行
    bool add_status_line(int status, const char* title);

    // 添加消息报头，具体的添加文本长度、连接状态和空行
    bool add_headers(int content_length);

    // 添加文本类型，这里是html
    bool add_content_type();

    // 添加Content-Length，表示响应报文的长度
    bool add_content_length(int content_length);

    // 添加连接状态，通知浏览器端是保持连接还是关闭
    bool add_linger();

    // 添加空行
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL*     mysql;

private:
    int         m_sockfd;
    sockaddr_in m_address;

    // 存储读取的请求报文数据
    char m_read_buf[READ_BUFFER_SIZE];

    // 缓冲区中m_read_buf中数据的最后一个字节的下一个位置
    int m_read_idx;

    // m_read_buf读取的位置m_checked_idx
    int m_checked_idx;

    // m_read_buf中已经解析的字符个数
    int m_start_line;

    // 存储发出的响应报文数据
    char m_write_buf[WRITE_BUFFER_SIZE];

    // 指示buffer中的长度
    int m_write_idx;

    // 主状态机的状态
    CHECK_STATE m_check_state;

    // 请求方法
    METHOD m_method;

    // 以下为解析请求报文中对应的6个变量
    char  m_real_file[FILENAME_LEN]; // 存储读取文件的名称
    char* m_url;
    char* m_version;
    char* m_host;
    int   m_content_length;
    bool  m_linger;

    char*       m_file_address; // 读取服务器上的文件地址
    struct stat m_file_stat;

    struct iovec m_iv[2]; // io向量机制iovec
    int          m_iv_count;
    int          cgi;             // 是否启用的  POST
    char*        m_string;        // 存储请求头数据
    int          bytes_to_send;   // 剩余发送字节数
    int          bytes_have_send; // 已发送字节数
};

#endif
