#include "lst_timer.h"
#include <iostream>

sort_timer_lst::~sort_timer_lst()
{
    util_timer* tmp = head;
    while (tmp)
    {
        head = head->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer* timer)
{
    if(!timer)
    {
        return;
    }
    if(!head)
    {
        head = tail = timer;
        return;
    }
    if(timer->expire < head->expire)
    {
        timer->next = head;
        head->pre = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

/*
    若目标定时器在链表尾部，或者目标定时器的超时时间仍然小于其下一个定时器的超时时间，则无需调整
    若目标定时器为头节点，则将其取出并重新插入链表
    否则，将目标定时器取出并重新插入链表
*/
void sort_timer_lst::adjust_timer(util_timer* timer)
{
    if(!timer)
    {
        return;
    }
    
    if(timer == tail || timer->expire < timer->next->expire)
    {
        return;
    }
    
    if(timer == head)
    {
        head = head->next;
        head->pre = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    
    else
    {
        timer->pre->next = timer->next;
        timer->next->pre = timer->pre;
        add_timer(timer, timer->next);
    }
}


void sort_timer_lst::del_timer(util_timer* timer)
{
    if(!timer)
    {
        return;
    }

    //若目标定时器为唯一节点，则删除后链表为空
    if(timer == head && timer == tail)
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }

    //若目标定时器为头节点，则将头节点后移
    if(timer == head)
    {
        head = head->next;
        head->pre = NULL;
        delete timer;
        return;
    }

    //若目标定时器为尾节点，则将尾节点前移
    if(timer == tail)
    {
        tail = tail->pre;
        tail->next = NULL;
        delete timer;
        return;
    }

    //若目标定时器在链表中间，则将其前后节点连接起来
    timer->pre->next = timer->next;
    timer->next->pre = timer->pre;
    delete timer;
}

/*
    每次SIGALRM信号被触发就在其信号处理函数中执行tick函数
    先获取当前系统时间并存于cur变量中
    遍历定时器链表，依次处理到期的定时器，直到遇到一个未到期的定时器为止
*/
void sort_timer_lst::tick()
{
    if(!head)
    {
        return;
    }

    std::cout<<"time tick"<<std::endl;
    
    time_t cur = time(NULL);
    util_timer* tmp = head;
    while(tmp)
    {
        if(cur < tmp->expire)
        {
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if(head) head->pre = NULL;
        delete tmp;
        tmp = head;
    }
}

/*
    将目标定时器timer插入到节点lst_head之后的部分链表中
*/
void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head)
{
    util_timer* pre = lst_head;
    util_timer* tmp = pre->next;

    //在当前列表中找第一个timer的超时时间大于目标定时器的位置并插入
    while(tmp)
    {
        if(timer->expire < tmp->expire)
        {
            pre->next = timer;
            timer->pre = pre;
            timer->next = tmp;
            tmp->pre = timer;
            break;
        }
        pre = tmp;
        tmp = tmp->next;
    }
    
    //遍历完列表却仍未找到则直接插入列表尾部
    if(!tmp)
    {
        pre->next = timer;
        timer->pre = pre;
        timer->next = NULL;
        tail = timer;
    }
}