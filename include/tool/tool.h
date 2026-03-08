#ifndef __TOOL_H__
#define __TOOL_H__

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "../hoop_timer/min_heap.h"
class time_heap;
class Utils
{
public:
    Utils(){}
    ~Utils(){}

    void init(int timeslot);

    int set_nonblock(int fd);

    void add_fd(int epollfd, int fd, bool one_shot, int TRIGMode);

    static void sig_handler(int sig);

    void addsig(int sig, void(handler)(int), bool restart = true);

    void timer_handler();

    void show_error(int connfd, const char* info);

public:
    static int* sig_pipefd;
    static int u_epollfd;
    int m_TIMESLOT;
    time_heap* m_time_heap;
};

int set_nonblock(int fd);
void add_fd(int epollfd, int fd, bool one_shot, int TRIGMode);
void remove_fd(int epollfd, int fd);
void mod_fd(int epollfd, int fd, int ev, int TRIGMode);

#endif