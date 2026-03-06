#ifndef __MIN_HEAP__
#define __MIN_HEAP__

#include <cstddef>
#include <exception>
#include <time.h>
#include <netinet/in.h>
#include <vector>
const int BUFFER_SIZE = 1024;

class heap_timer;

struct client_data
{
    struct sockaddr_in client_addr;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer* timer;
};

class heap_timer
{
public:
    heap_timer(int timeout)
    {
        expire = time(NULL) + timeout;
    }

public:
    time_t expire;
    void(*cb_func)(client_data* user_data);
    client_data* user_data;
};

class time_heap
{
public:
    time_heap(int num);
    time_heap(heap_timer** init_arr, int size, int capacity);
    ~time_heap();

public:
    void add_timer(heap_timer* timer);
    void del_timer(heap_timer* timer);
    heap_timer* top();
    void pop_timer();

private:
    void percolate_down(int hole);
    void resize();
    bool empty() const{return _cur_size == 0;}

private:
    std::vector<heap_timer*>* _arr;
    int _cur_size;   //当前包含定时器个数
    int _capacity;   //数组容量
};



#endif