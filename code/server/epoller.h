#ifndef EPOLLER_H_
#define EPOLLER_H_

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller {
public:
  explicit Epoller(int maxEvent = 1024);
  ~Epoller();

  bool AddFd(int fd, uint32_t events); // 添加 监听事件类型events的文件描述符为fd 
  bool ModFd(int fd, uint32_t events); // 更改 监听事件类型events的文件描述符为fd
  bool DelFd(int fd, uint32_t events); // 删除 监听事件类型events的文件描述符为fd
  
  int Wait(int timeoutMs = -1); // 监听等待epoll事件发生
  
  int GetEventFd(size_t idx) const; // 获取索引值idx对应的文件描述符

   uint32_t GetEvents(size_t idx) const; // 获取索引值idx的事件类型

private:
  int epollfd_; // epoller内核开辟的空间文件描述符
  std::vector<struct epoll_event> events_;
};


#endif // EPOLLER_H_