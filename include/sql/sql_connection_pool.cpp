#include "sql_connection_pool.h"
#include "../log/log.h"

connection_pool::connection_pool():m_CurConn(0),m_FreeConn(0)
{
}

connection_pool::~connection_pool()
{
    DestroryPool();
}

connection_pool* connection_pool::GetInstance()
{
    static connection_pool conn_Pool;
    return &conn_Pool;
}

void connection_pool::init(string url, string user, string password, string database_name, int port, int max_conn, int close_log)
{
    m_Url = url;
    m_Port = port;
    m_User = user;
    m_Password = password;
    m_DatabaseName = database_name;
    m_close_log = close_log;


    for(int i=0;i< max_conn;i++)
    {
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);

        if(conn == nullptr)
        {
            LOG_ERROR("MySQL Error");
            exit(1);
        }

        conn = mysql_real_connect(conn,url.c_str(),user.c_str(),password.c_str(),database_name.c_str(),port,NULL,0);

        if(conn == nullptr)
        {
            LOG_ERROR("MySQL Error");
            exit(1);
        }

        connList.push_back(conn);
        m_FreeConn++;
    }

    reserve = sem(m_FreeConn);
    m_MaxConn = m_FreeConn;
}


MYSQL* connection_pool::GetConnection()
{
    if(connList.size() == 0)
        return nullptr;
    
    MYSQL* conn;

    reserve.wait();
    lock.lock();

    conn = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();

    return conn;
}

bool connection_pool::ReleaseConnection(MYSQL* conn)
{
    if(conn == nullptr)
        return false;
    
    lock.lock();
    
    connList.push_back(conn);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    reserve.post();

    return true;
}

void connection_pool::DestroryPool()
{
    lock.lock();

    if(connList.size() > 0)
    {
        list<MYSQL*>::iterator it;
        for(it = connList.begin();it != connList.end(); ++it)
        {
            MYSQL* conn = *it;
            mysql_close(conn);
        }

        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }

    lock.unlock();
}

int connection_pool::GetFreeConn()
{
    return m_FreeConn;
}

connectionRAII::connectionRAII(MYSQL **conn, connection_pool* connPool)
{
    *conn = connPool->GetConnection();

    connRAII = *conn;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    if(poolRAII != nullptr)
        poolRAII->ReleaseConnection(connRAII);
}


