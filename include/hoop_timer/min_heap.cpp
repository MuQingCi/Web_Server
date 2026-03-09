#include "min_heap.h"
#include <cstddef>

static int epollfd;

void cb_func(client_data* user_data)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);

    close(user_data->sockfd);

    http_conn::m_user_count--;
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
    for(auto ptr : *_arr)
    {
        delete ptr;
    }
    delete _arr;
}


void time_heap::add_timer(heap_timer* timer)
{
    if(timer == nullptr) return;
    
    // 检查是否需要扩容
    if(_cur_size >= _capacity) resize();
    
    int hole = _cur_size;
    
    // 上浮调整 
    while (hole > 0)  // hole > 0 才需要比较父节点
    {
        int parent = (hole - 1) / 2;
        
        //确保父节点索引有效
        if (parent < 0 || parent >= _cur_size) break;
        
        //确保父节点指针不为空
        if (!(*_arr)[parent]) {
            fprintf(stderr, "Warning: null parent timer at %d\n", parent);
            break;
        }

        if ((*_arr)[parent]->expire <= timer->expire) {
            break;
        }

        (*_arr)[hole] = (*_arr)[parent];
    }

    (*_arr)[hole] = timer;
}

void time_heap::del_timer(heap_timer* timer)
{
    if(!timer) return;
    timer->m_cb_func = nullptr;
}

heap_timer* time_heap::top()
{
    if(empty()) return nullptr;
   return (*_arr)[0];
}

void time_heap::pop_timer()
{
    if(empty()) return;
    (*_arr)[0]->cb_func( (*_arr)[0]->user_data);
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
    
    while ((hole * 2 + 1) < size)  // 有左子节点
    {
        child = hole * 2 + 1;
        
        if((child < _cur_size-1) && ((*_arr)[child+1]->expire < (*_arr)[child]->expire)) ++child;

        if((*_arr)[child]->expire < tm->expire)
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
    std::vector<heap_timer*>*tmp = new std::vector<heap_timer*>(2 * _capacity, nullptr);

    for(int i=0;i<_cur_size;++i) (*tmp)[i] = (*_arr)[i];
    _capacity *= 2;
    delete []_arr;
    _arr = tmp;
}

void time_heap::tick()
{
    heap_timer* tmp = (*_arr)[0];
    time_t cur = time(NULL);
    while(!empty())
    {
        if(!tmp) break;
        if(tmp->expire > cur) break;
        if(tmp->cb_func) tmp->cb_func(tmp->user_data);
        pop_timer();
        tmp = (*_arr)[0];
    }
}

void time_heap::percolate_up(int hole)
{
    if (hole < 0 || hole >= _cur_size) return;
    
    heap_timer* timer = (*_arr)[hole];
    
    while (hole > 0) {
        int parent = (hole - 1) / 2;
        
        // 如果父节点小于等于当前节点，停止上浮
        if ((*_arr)[parent]->expire <= timer->expire) {
            break;
        }
        
        // 父节点下移
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
    
    // 1. 找到定时器在堆中的位置
    int pos = find_timer_pos(timer);
    
    if (pos == -1) {
        // 定时器不在堆中，直接添加
        timer->expire = new_expire;
        add_timer(timer);
        return;
    }
    
    // 2. 记录旧值
    time_t old_expire = timer->expire;
    
    // 3. 如果新旧值相同，不需要调整
    if (old_expire == new_expire) {
        return;
    }
    
    // 4. 更新过期时间
    timer->expire = new_expire;
    
    // 5. 根据大小关系调整位置
    if (new_expire < old_expire) {
        // 过期时间变小了，需要上滤
        percolate_up(pos);
    } else {
        // 过期时间变大了，需要下滤
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