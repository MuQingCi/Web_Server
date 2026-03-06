#ifndef __LST_TIMER__
#define __LST_TIMER__

#include <cstddef>
#include <ctime>
#include <time.h>
#include <netinet/in.h>

const int BUFFER_SIZE = 64;
class util_timer;

/*
    客户端数据结构体(含客户端地址、套接字描述符、读缓存以及定时器)
*/
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
};

/*
    定时器类(含前，后定时器指针pre,next、客户端结构体、任务超时时间以及对应的回调函数)
*/
class util_timer
{
public:
    util_timer():pre(NULL),next(NULL){}

public:
    time_t expire;
    void (*cb_func)(client_data*);
    client_data* user_data;
    util_timer* pre;
    util_timer* next;
};

/*
    升序双向且带有头尾节点的定时器列表
*/
class sort_timer_lst
{
public:
    sort_timer_lst():head(NULL),tail(NULL){}
    ~sort_timer_lst();
    void add_timer(util_timer* timer);
    void adjust_timer(util_timer* timer);
    void del_timer(util_timer* timer);
    void tick();

private:
    void add_timer(util_timer* timer, util_timer* lst_head);

private:
    util_timer* head;
    util_timer* tail;

};



#endif