#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <iostream>
#include <string.h>
#include "block_queue.h"

class Log
{
public:
    static Log* get_instance()
    {
        static Log log;
        return &log;
    }

    bool init(const char *file_name,int close_log, int log_buf_size = 8192,int split_lines = 5000000,int max_queue_size = 0);
    
    static void* flush_log_thraed(void *argc)
    {
        Log::get_instance()->async_write_log();
        return nullptr;
    }

    void write_log(int level,const char *format,...);

    void flush(void);

private:
    Log();
    virtual ~Log();

    void* async_write_log()
    {
        std::string single_log;

        while(m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(),m_fp);
            m_mutex.unlock();
        }
        return nullptr;
    }

private:
    char dir_name[128]; //路径
    char log_name[128]; //日志名称
    int m_split_lines; //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count; //日志行数记录
    int m_today; //日志记录的时间
    FILE *m_fp; //打开文件指针
    char *m_buf;
    block_queue<std::string> *m_log_queue; //阻塞队列
    bool m_is_async; //是否采用异步写入日志
    locker m_mutex;
    int m_close_log;
};

#define LOG_DEBUG(format,...) if(m_close_log == 0) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__);Log::get_instance()->flush();} 

#define LOG_INFO(format,...) if(m_close_log == 0) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__);Log::get_instance()->flush();} 

#define LOG_WARN(format,...) if(m_close_log == 0) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__);Log::get_instance()->flush();} 

#define LOG_ERROR(format,...) if(m_close_log == 0) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__);Log::get_instance()->flush();}

#endif

