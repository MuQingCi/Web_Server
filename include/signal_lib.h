#ifndef __SIGNAL_LIB_H__
#define __SIGNAL_LIB_H__

#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static int pipefd[2];

void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

void add_sig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));

    sa.sa_handler = handler;
    if(restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);

    assert(sigaction(sig,&sa,NULL) != -1);
}


#endif  