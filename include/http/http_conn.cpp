#include "http_conn.h"


locker m_lock;
map<string,string> users;

int http_conn::m_epollfd = -1;
int http_conn:: m_user_count = 0;


http_conn::http_conn()
{

}

void http_conn::initmysql_result(connection_pool* connPool)
{
    MYSQL* mysql = nullptr;
    connectionRAII mysqlConn(&mysql, connPool);

    if(mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error: %s\n", mysql_errno(mysql));
    }

    MYSQL_RES* result = mysql_store_result(mysql);

    int num_fields = mysql_num_fields(result);

    MYSQL_FIELD* fields = mysql_fetch_field(result);
    
    while(MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}


void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMod, int close_log, string user, string passwd, string sqlname)
{
    m_sockfd = sockfd;
    m_address = addr;
    m_TRIGMode = TRIGMod;
    m_close_log = close_log;
    doc_root = root;

    add_fd(m_epollfd, m_sockfd, true, m_TRIGMode);
    m_user_count++;

    strcpy(sql_name, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());
}

void http_conn::init()
{
    improv = 0;
    timer_flag = 0;

    mysql = nullptr;
    m_state = 0;

    m_read_idx = 0;
    m_checked_idx = 0;
    m_start_line = 0;

    m_write_idx = 0;

    m_check_state = CHECK_STATE_REQUESTLINT;
    m_method = GET;

    m_url = 0;
    m_version = 0;
    m_host = 0;
    m_linger = false;

    cgi = 0;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_close_log = 0;

    memset(m_read_buff, '\0', READ_BUFF_SIZE);
    memset(m_write_buff, '\0', WRITE_BUFF_SIZE);
    memset(m_read_buff, '\0', FILENAME_LEN);
}


void http_conn::close_conn(bool real_close)
{
    if(real_close && (m_sockfd != -1))
    {
        printf("close %d\n", m_sockfd);
        if(m_epollfd != -1)
            remove_fd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST)
    {
        mod_fd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        return;
    }
     
    bool write_ret = process_write(read_ret);
    if(!write_ret)
    {
        close_conn();
    }
    mod_fd(m_epollfd, m_sockfd, EPOLLOUT,m_TRIGMode);
}

bool http_conn::read_once()
{
    if(m_read_idx >= READ_BUFF_SIZE)
    {
        return false;
    }

    int byte_read = 0;

    if(m_TRIGMode == 0) //LT
    {
        byte_read = recv(m_sockfd, m_read_buff + m_read_idx, READ_BUFF_SIZE - m_read_idx, 0);
        
        if(byte_read <= 0)
        {
            return false;
        }

        m_read_idx += byte_read;

        return true;
    }

    else
    {
        while(true)
        {
            byte_read = recv(m_sockfd, m_read_buff + m_read_idx, READ_BUFF_SIZE - m_read_idx, 0);
            
            if(byte_read == -1)
            {
                if(errno == EAGAIN | errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if(byte_read == 0)
                return false;
            m_read_idx += byte_read;
        }
        return true;
    }
}


http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = nullptr;

    while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();

        m_start_line = m_checked_idx;
        
        switch ((m_check_state))
        {
        case CHECK_STATE_REQUESTLINT:
            ret = parse_request_line(text);
            if(ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        case CHECK_STATE_HEADER:
            ret = parse_request_headers(text);
            if(ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if(ret == GET_REQUEST)
                return do_request();
            break;

        case CHECK_STATE_CONTENT:
            ret = parse_content(text);
            if(ret == GET_REQUEST)
                return do_request();
            
            line_status = LINE_OPEN;
            break;

        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

http_conn::LINE_STATUS http_conn::parse_line()
{
    char tmp;
    for(;m_checked_idx < m_read_idx;++m_checked_idx)
    {
        tmp = m_read_buff[m_checked_idx];
        if(tmp == '\r')
        {
            if((m_checked_idx + 1) == m_read_idx)
                return LINE_OPEN;
            else if(m_read_buff[m_checked_idx + 1] == '\n')
            {
                m_read_buff[m_checked_idx++] = '\0';
                m_read_buff[m_checked_idx++] = '\0';
                return LINE_OK;
            }
        }

        else if(tmp == '\n')
        {
            if(m_checked_idx > 1 && m_read_buff[m_checked_idx - 1] == '\r')
            {
                m_read_buff[m_checked_idx-1] = '\0';
                m_read_buff[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}




http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    m_url = strpbrk(text, "\t");
    if(m_url == nullptr)
        return BAD_REQUEST;
    *m_url++ = '\0';

    char* method = text;
    if(strcasecmp(method, "GET") == 0)
        m_method = GET;
    
    else if(strcasecmp(method, "POST") == 0)
    {
        m_method = POST;
        cgi = 1;
    }

    else 
        return BAD_REQUEST;
    
    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    
    if(m_version == nullptr) 
        return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    
    if(strcasecmp(m_version, "HTTP/1.1") != 0) 
        return BAD_REQUEST;
    if(strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if(strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if(m_url == nullptr || m_url[0] != '/')
        return BAD_REQUEST;
    
    if(strlen(m_url) == 1)
        strcat(m_url, "judge.html");

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_request_headers(char* text)
{
    if(text[0] == '\0')
    {
        if(m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;

        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0)
            m_linger = true;
    }

    else if(strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    
    else if(strncasecmp(text, "HOST:", 5) == 0
 
    

}






