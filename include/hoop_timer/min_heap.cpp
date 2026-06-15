#include "min_heap.h"
#include "../tool/tool.h"
#include "../http/http_conn.h"

void cb_func(client_data* user_data)
{
    if(user_data == nullptr) return;
    
    int epollfd = Utils::u_epollfd;
    if(epollfd > 0 && user_data->sockfd > 0)
    {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
        close(user_data->sockfd);
    }
    
    http_conn::m_user_count--;
    
    // 将 client_data 中的 sockfd 置为已关闭状态
    user_data->sockfd = -1;
}


time_heap::time_heap(int num):_capacity(num),_cur_size(0)
    {
        _arr = new std::vector<heap_timer*>(_capacity, nullptr);
    }

time_heap::time_heap(heap_timer** init_arr, int size, int capacity)
    :_cur_size(size),_capacity(capacity)
    {
        if(capacity < size)
        {
            throw std::exception();
        }
        _arr = new std::vector<heap_timer*>(capacity,nullptr);
        if(!_arr) throw(std::exception());
        for(int i=0;i<size;++i)
        {
            (*_arr)[i] = init_arr[i];
        }
        for(int i=(_cur_size-1)/2;i>0;++i)
        {
            percolate_down(i);
        }
    }

time_heap::~time_heap()
{
    for(int i = 0; i < _cur_size; i++) {
        delete (*_arr)[i];
    }
    delete _arr;
}


void time_heap::add_timer(heap_timer* timer)
{
    if(timer == nullptr) return;
    
    // 检查是否需要扩容
    if(_cur_size >= _capacity) resize();
    
    int hole = _cur_size;
    _cur_size++;
    
    // 上浮调整 
    int parent;
    while (hole > 0)
    {
        parent = (hole - 1) / 2;
        
        if (!(*_arr)[parent]) {
            break;
        }

        if ((*_arr)[parent]->expire <= timer->expire) {
            break;
        }

        (*_arr)[hole] = (*_arr)[parent];
        hole = parent;
    }

    (*_arr)[hole] = timer;
}

void time_heap::del_timer(heap_timer* timer)
{
    if(!timer) return;
    // 惰性删除：将回调函数置空，tick() 时会跳过并清理
    timer->cb_func = nullptr;
}

heap_timer* time_heap::top()
{
    if(empty()) return nullptr;
   return (*_arr)[0];
}

void time_heap::pop_timer()
{
    if(empty()) return;
    
    // pop_timer 只负责堆结构调整，不调用 cb_func
    // cb_func 已经在 tick() 中被调用过了
    delete (*_arr)[0];

    (*_arr)[0] = (*_arr)[--_cur_size];
    percolate_down(0);
}
 

void time_heap::percolate_down(int hole)
{
    if (hole < 0 || hole >= _cur_size) {
        return;
    }
    
    heap_timer* timer = (*_arr)[hole];
    if (!timer) {
        return;
    }
    
    int child;
    int size = _cur_size;
    
    while ((hole * 2 + 1) < size)
    {
        child = hole * 2 + 1;
        
        if((child < _cur_size-1) && ((*_arr)[child+1] != nullptr) && ((*_arr)[child] != nullptr)
            && ((*_arr)[child+1]->expire < (*_arr)[child]->expire)) ++child;

        if((*_arr)[child] != nullptr && timer != nullptr 
            && (*_arr)[child]->expire < timer->expire)
        {
            (*_arr)[hole] = (*_arr)[child];
            hole = child;
        }
        else
        {
            break;
        }
    }
    
    (*_arr)[hole] = timer;
}


void time_heap::resize()
{
    std::vector<heap_timer*>* tmp = new std::vector<heap_timer*>(2 * _capacity, nullptr);

    for(int i=0;i<_cur_size;++i) (*tmp)[i] = (*_arr)[i];
    _capacity *= 2;
    delete _arr;
    _arr = tmp;
}

void time_heap::tick()
{
    time_t cur = time(NULL);
    while(!empty())
    {
        heap_timer* tmp = (*_arr)[0];
        if(!tmp) {
            // 空指针不应该出现在堆中，但做防御性检查
            pop_timer();
            continue;
        }
        
        // 惰性删除的定时器：cb_func 为 nullptr，直接弹出
        if(tmp->cb_func == nullptr) {
            delete (*_arr)[0];
            (*_arr)[0] = (*_arr)[--_cur_size];
            percolate_down(0);
            continue;
        }
        
        if(tmp->expire > cur) break;
        
        // 调用回调函数（会在 cb_func 中关闭连接 + 减少 m_user_count）
        tmp->cb_func(tmp->user_data);
        
        // pop_timer 只释放内存并调整堆，不再重复调用 cb_func
        delete (*_arr)[0];
        (*_arr)[0] = (*_arr)[--_cur_size];
        percolate_down(0);
    }
}

void time_heap::percolate_up(int hole)
{
    if (hole < 0 || hole >= _cur_size) return;
    
    heap_timer* timer = (*_arr)[hole];
    if (!timer) return;
    
    while (hole > 0) {
        int parent = (hole - 1) / 2;
        
        if (!(*_arr)[parent]) break;
        
        if ((*_arr)[parent]->expire <= timer->expire) break;
        
        (*_arr)[hole] = (*_arr)[parent];
        hole = parent;
    }

    (*_arr)[hole] = timer;
}


void time_heap::adjust_timer(heap_timer* timer, time_t new_expire)
{
    if (!timer) {
        return;
    }
    
    int pos = find_timer_pos(timer);
    
    if (pos == -1) {
        timer->expire = new_expire;
        add_timer(timer);
        return;
    }
    
    time_t old_expire = timer->expire;
    
    if (old_expire == new_expire) {
        return;
    }
    
    timer->expire = new_expire;
    
    if (new_expire < old_expire) {
        percolate_up(pos);
    } else {
        percolate_down(pos);
    }
}

int time_heap::find_timer_pos(heap_timer* timer)
{
    if (!timer) return -1;
    
    for (int i = 0; i < _cur_size; i++) {
        if ((*_arr)[i] == timer) {
            return i;
        }
    }
    return -1;
}