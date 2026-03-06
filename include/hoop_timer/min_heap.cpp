#include "min_heap.h"
#include <cstddef>

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
        delete [] _arr;
    }


void time_heap::add_timer(heap_timer* timer)
{
    int hole = ++_cur_size;
    int parent = 0;
    for(;hole >= 0; hole = parent)
    {
        parent = (_cur_size-1)/2;
        if((*_arr)[parent]->expire <= timer->expire) break;
        (*_arr)[hole] = (*_arr)[parent];
    }
    (*_arr)[hole] = timer;
}

void time_heap::del_timer(heap_timer* timer)
{
    if(!timer) return;
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
    (*_arr)[0]->cb_func( (*_arr)[0]->user_data);
    delete (*_arr)[0];

    (*_arr)[0] = (*_arr)[--_cur_size];
    percolate_down(0);
}
 

void time_heap::percolate_down(int hole)
{
    heap_timer* tm = (*_arr)[hole];
    int child = 0;
    for(;(hole *2 +1) <= (_cur_size-1);hole = child)
    {
        child = hole * 2 + 1;
        
        if((child < _cur_size-1) && ((*_arr)[child+1]->expire < (*_arr)[child+1]->expire)) ++child;

        if((*_arr)[child]->expire < tm->expire)
        {
            (*_arr)[hole] = (*_arr)[child];
        }

        else break;
    }
    
    (*_arr)[hole] = tm;
}


void time_heap::resize()
{
    std::vector<heap_timer*>*tmp = new std::vector<heap_timer*>(2 * _capacity, nullptr);

    for(int i=0;i<_cur_size;++i) (*tmp)[i] = (*_arr)[i];
    _capacity *= 2;
    delete []_arr;
    _arr = tmp;
}