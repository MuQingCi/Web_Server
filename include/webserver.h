#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <arpa/inet.h>
#include "./http/http_conn.h"
#include "./sql/sql_connection_pool.h"
#include "./thread_pool/thread_pool.h"
#include "./hoop_timer/min_heap.h"
#include "./log/log.h"
#include "./tool/tool.h"

const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;
const int MAX_FD = 65536;

class WebServer
{
public:
    WebServer();
    ~WebServer();
    
    void init(int port, string user, string passwd, string database_name, 
              int log_write, int opt_linger, int trigmode, 
              int sql_num, int thread_num);
    
    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(heap_timer* timer);
    void deal_timer(heap_timer* timer, int sockfd);
    bool deal_client_data();
    bool deal_with_signal(bool& timeout, bool& stop_server);
    void deal_with_read(int sockfd);
    void deal_with_write(int sockfd);

public:
    //基础
    int m_port;
    char* m_root;
    int m_log_write;
    int m_close_log;
    int m_actor_model;

    int m_pipefd[2];
    int m_epollfd;
    http_conn* users;

    //数据库
    connection_pool* m_connPool;
    string m_user;
    string m_passwd;
    string m_databaseName;
    int m_sql_num;

    //线程池
    threadpool<http_conn>* m_pool;
    int m_thread_num;

    //epoll
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;
    
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;

    //定时器
    client_data* users_timer;
    Utils utils;
};

#endif