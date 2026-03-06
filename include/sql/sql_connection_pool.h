#ifndef __SQL_CONNECTION_POOL_H__
#define __SQL_CONNECTION_POOL_H__

#include <mysql/mysql.h>
#include <string.h>
#include <string>
#include <list>
#include "../lock/locker.h"

using namespace std;

class connection_pool
{
public:
    MYSQL *GetConnection();                 //获取数据库连接
    bool ReleaseConnection(MYSQL *conn);    //释放连接
    int GetFreeConn();                      //获取空闲连接
    void DestroryPool();                    //销毁所有连接

    static connection_pool* GetInstance();

    void init(string url, string user, string password, string database_name, int port,
    int max_conn, int close_log);

private:
    connection_pool();
    ~connection_pool();

private:
    int m_MaxConn;
    int m_CurConn;
    int m_FreeConn;
    locker lock;
    list<MYSQL*> connList;
    sem reserve;

public:
    string m_Url;
    string m_Port;
    string m_User;
    string m_Password;
    string m_DatabaseName;
    int m_close_log;
};

class connectionRAII
{
public:
    connectionRAII(MYSQL** conn, connection_pool* connnPool);
    ~connectionRAII();

private:
    MYSQL* connRAII;
    connection_pool* poolRAII; 
};

#endif