#ifndef __TIME_WHEEL_TIMER__
#define __TIME_WHEEL_TIMER__

#include <netinet/in.h>

const int BUFFER_SIZE = 64;

class tw_timer;

struct client_data
{
public:
    sockaddr_in sockaddr;
    int sockfd;
    char buf[BUFFER_SIZE];
    tw_timer* timer;

};

class tw_timer
{
public:
    tw_timer(int rot,int ts):pre(nullptr),next(nullptr),rotation(rot)
    ,time_slot(ts){};

public:
    void (*cb_func)(client_data*);
    int rotation; //需要等待的圈数
    int time_slot; //位于时间轮上的位置
    client_data* user_data;
    tw_timer* pre;
    tw_timer* next;
};


class time_wheel
{
public:
    time_wheel():cur_slot(0)
    {
        for(int i=0;i<N;++i)
        {
            slots[i] = nullptr;
        }
    }

    ~time_wheel();
    tw_timer* add_timer(int timeout);
    void del_timer(tw_timer* timer);
    void tick();

public:
    static const int N=60;  //总槽数
    static const int SI=1; //多久转一个槽
    tw_timer* slots[N]; //时间轮上的槽数
    int cur_slot;   //当前所处槽数

};

#endif