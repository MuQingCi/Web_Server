#ifndef __UTIL_H__
#define __UTIL_H__

void setnonblocking(int fd);

void addfd(int epollfd, int fd);

void delfd(int epollfd, int fd);

#endif // __UTIL_H__