# Tiny Web Server

[![C++11](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![Build](https://img.shields.io/badge/build-cmake-green.svg)](CMakeLists.txt)

从零实现的一款轻量级高性能 Web 服务器，基于 **epoll** 事件驱动架构，支持 **Reactor/Proactor 双模式运行时切换**，可承载千级并发连接，满足小型静态/动态 Web 服务需求。

---

## 目录

- [特性](#特性)
- [技术栈](#技术栈)
- [项目架构](#项目架构)
- [模块详解](#模块详解)
  - [I/O 模型与事件驱动](#io-模型与事件驱动)
  - [HTTP 解析](#http-解析)
  - [定时器管理](#定时器管理)
  - [线程池与数据库连接池](#线程池与数据库连接池)
  - [日志系统](#日志系统)
- [快速开始](#快速开始)
  - [环境要求](#环境要求)
  - [编译与运行](#编译与运行)
  - [命令行参数](#命令行参数)
- [目录结构](#目录结构)

---

## 特性

- **Reactor / Proactor 双 I/O 模型** — 运行时可动态切换，适应不同场景
- **epoll (LT / ET 模式组合)** — 支持四种触发模式组合，灵活控制事件通知
- **HTTP/1.1** — 自实现有限状态机（FSM）解析 HTTP 请求，支持 GET / POST 方法及 keep-alive 长连接
- **mmap 零拷贝** — 静态文件响应经由内存映射直接发送，减少内核态与用户态数据拷贝
- **最小堆定时器** — O(log n) 时间复杂度管理海量空闲连接，高效剔除超时连接
- **MySQL 连接池** — 可配置连接数，信号量 + 互斥锁保障线程安全
- **线程池** — 支持动态任务分发，Reactor 模式下处理读写事件，Proactor 模式下处理业务逻辑
- **异步日志系统** — 基于阻塞队列实现异步写入，支持按天 / 按大小自动切割，日志不阻塞请求处理
- **信号驱动** — 集成信号处理机制，实现定时器到期触发与服务优雅退出

---

## 技术栈

| 类别 | 技术 |
| --- | --- |
| 语言标准 | C++11 |
| I/O 多路复用 | epoll (LT / ET 模式) |
| 并发编程 | 多线程，pthread 线程池 |
| 网络协议 | TCP/IP，HTTP/1.1 |
| 数据库 | MySQL（连接池封装） |
| 内存管理 | mmap 零拷贝文件映射 |
| 定时器 | 最小堆（min-heap） |
| 日志 | 异步阻塞队列日志系统 |
| 构建工具 | CMake |
| 信号处理 | SIGALRM / SIGTERM / SIGPIPE |

---

## 项目架构

```
┌─────────────────────────────────────────┐
│                main.cpp                 │
│         (解析参数、初始化组件)            │
└───────────────┬─────────────────────────┘
                │
┌───────────────▼─────────────────────────┐
│              WebServer                  │
│  ┌──────────┬──────────┬──────────────┐ │
│  │  epoll   │ 信号驱动  │  定时器管理   │ │
│  │ 事件循环  │ 信号处理  │ 最小堆定时器  │ │
│  └────┬─────┴────┬─────┴──────┬───────┘  │
│       │          │            │          │
│  ┌────▼────┐ ┌───▼────┐ ┌────▼────────┐  │
│  │ 线程池  │ │ 连接池  │ │  HTTP 连接  │  │
│  │(Reactor │ │ (MySQL)│ │ (FSM 解析)  │  │
│  │Proactor)│ │        │ │ (mmap 响应) │  │
│  └─────────┘ └────────┘ └─────────────┘  │
└──────────────────────────────────────────┘
```

---

## 模块详解

### I/O 模型与事件驱动

- 基于 **epoll** 构建非阻塞事件循环，监听 fd 支持 **LT（水平触发）** 与 **ET（边缘触发）** 两种模式
- 可配置四种组合：监听 LT + 连接 LT / 监听 LT + 连接 ET / 监听 ET + 连接 LT / 监听 ET + 连接 ET
- **Reactor 模式（`--actor=1`）**：线程池负责读写数据和执行业务逻辑；主线程只负责事件分发
- **Proactor 模式（`--actor=0`）**：主线程负责读写完成数据，线程池只专注业务处理
- 集成 **信号驱动** 机制：通过 `socketpair` 将信号统一转发到 epoll，处理 SIGALRM（定时触发）和 SIGTERM（优雅退出）

```cpp
void WebServer::eventLoop()
{
    // epoll_wait 等待事件
    int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
    // 处理监听、读写、信号事件 ...
}
```

### HTTP 解析

- 自实现 **有限状态机（FSM）** 解析 HTTP/1.1 请求
- 三个解析阶段：`CHECK_STATE_REQUESTLINE` → `CHECK_STATE_HEADER` → `CHECK_STATE_CONTENT`
- 支持方法：**GET**、**POST**（含表单参数解析与用户注册/登录验证）
- 行解析状态：`LINE_OK` / `LINE_BAD` / `LINE_OPEN`
- 支持 **keep-alive** 长连接，解析 Connection 头部

### 定时器管理

- 基于**最小堆（min-heap）**实现定时器管理器，事件复杂度 O(log n)
- 支持定时器添加、删除、调整过期时间
- 每个连接对应一个 `heap_timer`，超时后自动关闭连接并释放资源
- 惰性删除策略：通过清空回调函数指针避免在堆中复杂删除操作

### 线程池与数据库连接池

**线程池：**
- 模板化线程池设计，支持任意任务类型
- 支持限流：最大请求队列上限可配置
- Reactor 模式下根据 `m_state` 区分读写操作分发给线程
- Proactor 模式下线程池只负责业务逻辑

**数据库连接池：**
- 单例模式 + RAII 封装，确保连接资源安全释放
- 支持可配置连接数量
- 使用信号量和互斥锁实现线程安全获取/释放

```cpp
connectionRAII mysql(&request->mysql, m_conn_Pool);  // RAII 自动归还连接池
```

### 日志系统

- 基于 **阻塞队列（block_queue）** 实现异步写入
- 支持**同步写入**和**异步写入**两种模式
- 日志文件支持**按天自动切割**（`m_today` 日期检测）和**按行数切割**（`m_split_lines`）
- 通过宏定义提供分级日志：`LOG_DEBUG` / `LOG_INFO` / `LOG_WARN` / `LOG_ERROR`
- 日志写入操作独立于请求处理线程，不影响服务响应性能

---

## 快速开始

### 环境要求

- Linux (内核版本 ≥ 2.6.27，支持 epoll)
- CMake ≥ 3.10
- MySQL / MariaDB 数据库
- MySQL C++ Connector（`mariadb-connector-cpp` 或 `mysql-connector-c++`）

### 编译与运行  
注:需自备mp4以及jpg文件
```bash
# 1. 克隆项目
git clone https://github.com/MuQingCi/Web_Server.git
cd Tiny_Web_Server

# 2. 构建
mkdir build && cd build
cmake ..
make

# 3. 初始化数据库
mysql -u root -p < your_db_init.sql

# 4. 启动服务器（默认端口 9006）
./bin/TinyWebServer

# 或带参数启动
./bin/TinyWebServer -p 9006 -m 0 -a 0 -s 8 -t 8
```

### 命令行参数

| 参数 | 含义 | 默认值 |
| --- | --- | --- |
| `-p` | 端口号 | 9006 |
| `-l` | 日志写入方式（0 同步 / 1 异步） | 0 |
| `-m` | 触发模式组合（0~3） | 0 |
| `-o` | 优雅关闭连接选项 | 0 |
| `-s` | 数据库连接池数量 | 8 |
| `-t` | 线程池数量 | 8 |
| `-c` | 是否关闭日志（0 开启 / 1 关闭） | 0 |
| `-a` | 模型选择（0 Proactor / 1 Reactor） | 0 |

`-m` 模式说明：
- `0`：监听 LT + 连接 LT
- `1`：监听 LT + 连接 ET
- `2`：监听 ET + 连接 LT
- `3`：监听 ET + 连接 ET

---

## 目录结构

```
Tiny_Web_Server/
├── CMakeLists.txt                 # CMake 构建配置
├── build.sh                       # 编译脚本
├── src/
│   └── main.cpp                   # 入口文件（解析参数、初始化）
├── include/
│   ├── config.h / config.cpp      # 配置解析（命令行参数）
│   ├── webserver.h / webserver.cpp # 核心服务器（事件循环、连接管理）
│   ├── http/
│   │   ├── http_conn.h / .cpp     # HTTP 连接处理（FSM 解析、mmap 响应）
│   ├── hoop_timer/
│   │   ├── min_heap.h / .cpp      # 最小堆定时器
│   ├── thread_pool/
│   │   └── thread_pool.h          # 线程池（模板类）
│   ├── sql/
│   │   ├── sql_connection_pool.h / .cpp  # MySQL 连接池
│   ├── log/
│   │   ├── log.h / log.cpp        # 日志系统
│   │   └── block_queue.h          # 阻塞队列（异步日志）
│   ├── lock/
│   │   └── locker.h               # 线程同步封装（互斥锁、信号量、条件变量）
│   ├── tool/
│   │   └── tool.h / tool.cpp      # 工具函数（epoll 操作、信号处理）
│   ├── list_timer/                # 双链表定时器（旧版，保留参考）
│   └── time_wheel/                # 时间轮定时器（旧版，保留参考）
└── source/                        # 静态资源文件（HTML、图片、视频等）
```

---

## 致谢

该项目参考了经典开源高并发服务器项目 [TinyWebServer](https://github.com/qinguoyi/TinyWebServer) 的设计思路，并在此基础上进行了独立实现与优化。

---

*如有任何问题或建议，欢迎提交 [Issue](https://github.com/MuQingCi/Web_Server/issues) 或 Pull Request。*