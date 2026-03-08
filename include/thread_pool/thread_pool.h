#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include<pthread.h>
#include"../lock/locker.h"
#include<exception>
#include<list>
#include"../sql/sql_connection_pool.h"


template<typename T>
class threadpool
{
public:

    threadpool(int actor_model, connection_pool *conn_Pool, int thread_number = 8, 
        int max_requests =  10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    static void *worker(void *argc);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //线程池中的最大请求数
    pthread_t *m_threads;       //线程数组
    std::list<T *> m_work_queue; //请求队列
    locker m_queue_locker;       //队列锁
    sem m_queue_stat;           //是否有任务需要处理
    connection_pool *m_conn_Pool;    //连接池
    int m_actor_model;          //切换模式
};

template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *conn_Pool, int thread_number, 
    int max_requests):m_actor_model(actor_model), m_thread_number(thread_number),
    m_max_requests(max_requests),m_conn_Pool(conn_Pool)
{
    if(m_thread_number <= 0 || m_max_requests <= 0) 
        throw std::exception();
    
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads) 
        throw std::exception();

    for(int i=0;i<m_thread_number;i++)
    {
        if(pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    if(m_threads != NULL)
    {
        delete[] m_threads;
    }
}

template<typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queue_locker.lock();
    if(m_work_queue.size() >= m_max_requests)
    {
        m_queue_locker.unlock();
        return false;
    }

    request->m_state = state;
    m_work_queue.push_back(request);
    m_queue_locker.unlock();
    m_queue_stat.post();

    return true;
}   

template<typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queue_locker.lock();
    if(m_work_queue.size() >= m_max_requests)
    {
        m_queue_locker.unlock();
        return false;
    }

    m_work_queue.push_back(request);
    m_queue_locker.unlock();
    m_queue_stat.post();

    return true;
}

template<typename T>
void* threadpool<T>::worker(void* argc)
{
    threadpool* pool = (threadpool*) argc;
    pool->run();
    return pool; 
}

template<typename T>
void threadpool<T>::run()
{
    while(true)
    {
        m_queue_stat.wait();
        m_queue_locker.lock();
        if(m_work_queue.empty())
        {
            m_queue_locker.unlock();
            continue;
        }

        T* request = m_work_queue.front();
        m_work_queue.pop_front();
        m_queue_locker.unlock();

        if(!request)
            continue;
        if(m_actor_model == 1)
        {
            if(request->m_state == 0)
            {
                if(request->read_once())
                {
                    request->improv = 1;
                    connectionRAII mysql(&request->mysql, m_conn_Pool);
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if(request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            connectionRAII mysql(&request->mysql, m_conn_Pool);
            request->process();
        }
    }
}

#endif
