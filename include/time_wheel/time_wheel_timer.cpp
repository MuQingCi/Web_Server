#include "time_wheel_timer.h"
#include <iostream>

time_wheel::~time_wheel()
{
    for(int i=0;i<N;++i)
    {
        if(!slots[i]) continue;
        tw_timer* tmp = slots[i];
        while (tmp) {
            slots[i] = tmp->next;
            delete tmp;
            tmp = slots[i];
        }
    }
}

tw_timer* time_wheel::add_timer(int timerout)
{
    if(timerout < 0) return nullptr;

    int ticks = 0;

    if(timerout < SI) ticks = 1;

    else
    {
        ticks = timerout / SI;
    } 

    int rot = ticks / N;
    int ts = ((ticks % N) + cur_slot) % N;

    tw_timer* timer = new tw_timer(rot,ts);

    if(!slots[ts])
    {
        std::cout <<"the timer is added in " << ts 
        << " time_slot, watting "<< rot <<" rotations." 
        <<std::endl;
        slots[ts] = timer;
    }
    else
    {
        timer->next = slots[ts];
        slots[ts]->pre = timer;
        slots[ts] = timer;
    }
    return timer;
}

void time_wheel::del_timer(tw_timer* timer)
{
    if(!timer) return;

    int ts = timer->time_slot;

    if(slots[ts] == timer)
    {
        slots[ts] = slots[ts]->next;

        if(slots[ts]) slots[ts]->pre = nullptr;
    }
    else
    {
        timer->pre->next = timer->next;

        if(timer->next) timer->next->pre = timer->pre;
    }
    delete timer;
    std::cout << "timer has deleted"<< std::endl;
}

void time_wheel::tick()
{
    tw_timer* tmp = slots[cur_slot];
    std::cout<<"cur_slot is " << cur_slot <<std::endl;
    
    while (tmp) 
    {
        std::cout<< "tick the timer once" << std::endl;
        if(tmp->rotation>0)
        {
            tmp->rotation--;
            tmp = tmp->next;
            continue;
        }
        
        tmp->cb_func(tmp->user_data);

        //若tmp为当前槽的头节点时
        if(tmp == slots[cur_slot])
        {
            slots[cur_slot] = tmp->next;
            delete tmp;
            if(slots[cur_slot]) slots[cur_slot] ->pre = nullptr;
            tmp = slots[cur_slot];
        }
        else    //tmp为链表中其他节点
        {
            tmp->pre->next = tmp->next;
            if(tmp->next) tmp->next->pre = tmp->pre;
            tw_timer* tmp2 = tmp->next;
            delete tmp;
            tmp = tmp2;
        }
    }

    cur_slot = ++cur_slot % N;  //更新当前时间槽
}