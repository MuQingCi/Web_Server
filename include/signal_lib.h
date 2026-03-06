#ifndef __SIGNAL_LIB__
#define __SIGNAL_LIB__

#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static int pipefd[2];

void sig_handler(int sig);
void add_sig(int sig);


#endif  