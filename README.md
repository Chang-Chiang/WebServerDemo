# WebServerDemo

[![standard-readme compliant](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/Chang-Chiang/WebServerDemo)

Simple Demo of Web Server

> [一文读懂社长的TinyWebServer](https://huixxi.github.io/2020/06/02/小白视角：一文读懂社长的TinyWebServer/)
>
> [TinyWebServer: Linux下C++轻量级Web服务器学习](https://github.com/qinguoyi/TinyWebServer)

+ 使用**线程池 + epoll(ET和LT均实现) + 模拟Proactor模式**的并发模型
+ 使用**状态机**解析HTTP请求报文，支持解析**GET和POST**请求
+ 通过访问服务器数据库实现web端用户**注册、登录**功能，可以请求服务器**图片和视频文件**
+ 实现**同步/异步日志系统**，记录服务器运行状态
+ 经Webbench压力测试可以实现**上万的并发连接**数据交换

## 目录

+ [Background](#background)

+ [Install](#install)
+ [Usage](#usage)
+ [Docs](#docs)
+ [License](#license)

## Background

+ 啃完 《TCP/IP 网络编程》 和 《Linux 高性能服务器编程》 后实战项目练手
+ 找工作太卷了
+ Linux 下 C++ 服务器开发

## Install 

```bash
$ git clone https://github.com/Chang-Chiang/WebServerDemo.git
```

## Usage

> [webserver的使用与配置](https://blog.csdn.net/yingLGG/article/details/121400284)

+ install MySQL

  ```bash
  # 安装
  $ sudo apt-get update
  $ sudo apt install libmysqlclient-dev  # <mysql/mysql.h>
  $ sudo apt-get install mysql-server  # 安装 mysql 服务
  
  # 初始化配置
  $ sudo mysql_secure_installation
  
  # 检查状态  
  # wsl 下无法使用 systemctl 命令, 换为 service
  # sudo service mysql status  # wsl 下 查看 mysql 状态
  # sudo service mysql start  # wsl 下启动 mysql
  $ systemctl status mysql.service
  
  # 进入 MySQL
  sudo mysql -uroot -p
  ```

+ create database

  ```sql
  # 建立yourdb库
  create database [dbname];
  
  # 创建 user 表
  USE [dbname];
  
  CREATE TABLE user(
      username char(50) NULL,
      passwd char(50) NULL
   )ENGINE=InnoDB;
  
  # 添加数据
  INSERT INTO user(username, passwd) VALUES('name', 'passwd');
  
  # 可以查看当前的数据库
  show databases; 
  ```

+ modify main.c

  ```bash
  $ cd /etc/mysql
  $ sudo vim debian.cnf
  ```

  ```c++
  // root root修改为服务器数据库的登录名和密码
  // [dbname] 修改为上述创建的 [dbname] 库名
  connPool->init("localhost", "root", "root", "[dbname]", 3306, 8);
  // connPool->init("localhost", "debian-sys-maint", "8tMp4GgzNQ7DtCo7", "web_server_demo", 3306, 8);
  ```

+ modify http_conn.cpp

  ```c++
  // 修改为 root 文件夹所在路径
  const char* doc_root="/home/[username]/WebServerDemo/root";
  ```

+ make

  ```bash
  $ make server
  ```

+ start

  ```bash
  $ ./server [port]
  ```

+ 浏览器端

  ```bash
  [ip]:[port]
  ```

## Docs

- [x] [运行流程剖析](./docs/01_运行流程剖析.md)
- [x] [事件处理线程池](./docs/02_半同步半反应堆线程池.md)
- [x] [数据库连接池](./docs/03_数据库连接池.md)
- [ ] [RAII](./DOCS/04_RAII.md)
- [x] [HTTP 连接处理类](./docs/05_http 连接处理类.md)
- [x] [日志系统](./docs/06_同步异步日志系统.md)
- [x] [定时器处理非活动连接](./docs/07_定时器处理非活动连接.md)
- [ ] [单元测试](./docs/08_单元测试.md)
- [x] [服务器压力测试](./docs/服务器压力测试.md)
- [x] [运行记录](./docs/10_运行记录.md)
- [ ] [问题记录](./docs/11_问题记录.md)

## License

[MIT © Chang Chiang.](../LICENSE)
