#include "../include/config.h"
#include <iostream>

int main(int argc, char* argv[])
{
    string user = "lanxiyuan";
    string passwd = "123456";
    string databasename = "sqqy_db";

    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    server.init(config.PORT, user, passwd, databasename, config.LOGWrite,
                 config.OPT_LINGER, config.TRIGMode, config.sql_num, 
                 config.thread_num);

    std::cout<<"init() run successfully!"<< std::endl;

    server.log_write();

    std::cout<<"log_write() run successfully!"<< std::endl;

    server.sql_pool();

    std::cout<<"sql_pool() run successfully!"<< std::endl;

    server.thread_pool();

    std::cout<<"thread_pool() run successfully!"<< std::endl;

    server.trig_mode();

    std::cout<<"trig_mode() run successfully!"<< std::endl;

    server.eventListen();
    
    std::cout<<"eventListen() run successfully!"<< std::endl;

    server.eventLoop();

    std::cout<<"eventLoop() run successfully!"<< std::endl;

    return 0;
}