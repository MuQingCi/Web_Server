#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <unistd.h>
#include <cstdlib>
#include "./webserver.h"

class Config
{
public:
    Config();
    ~Config(){}

    void parse_arg(int argc, char* argv[]);

    int PORT;

    int LOGWrite;

    int TRIGMode;

    int LISTENTrigmode;

    int CONNTrigmode;

    int OPT_LINGER;

    int sql_num;

    int thread_num;

    int close_log;

    int actor_model;

};


#endif