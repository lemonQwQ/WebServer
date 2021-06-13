#ifndef HTTPCONN_H_
#define HTTPCONN_H_

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
  HttpConn();
  ~HttpConn();

  void init(int sockFd, const sockaddr_in& addr);

  ssize_t read(int *saveErrno);  // 读取客户端数据

  ssize_t write(int *saveErrno); // 发送给客户端数据

  void Close();  // 关闭此链接

  int GetFd() const; // 获取当前连接对应的文件描述符

  int GetPort() const; // 获取当前连接对应的port

  const char* GetIP() const; // 获取当前连接对应的ip

  sockaddr_in GetAddr() const; //  获取当前连接对应的addr

  bool process(); // 处理当前连接

  // 返回输出数据大小
  int ToWriteBytes() { 
    return iov_[0].iov_len + iov_[1].iov_len;
  }

  bool isKeepAlive() const {
    return request_.IsKeepAlive(); 
  }

  static bool isET; // 连接以ET模式处理?
  static const char* srcDir; // 资源目录
  static std::atomic<int> userCount; // 当前客户端连接数
private:
  int fd_;
  struct sockaddr_in addr_;

  bool isClose_;

  int iovCnt_;
  struct iovec iov_[2];

  Buffer readBuff_;
  Buffer writeBuff_;

  HttpRequest request_;
  HttpResponse response_;
};

#endif