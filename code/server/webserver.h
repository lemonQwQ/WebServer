#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../timer/heaptimer.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnRAII.h"
#include "../pool/threadpool.h"

class WebServer {
public:
  WebServer(
    int port, int EventMode, int timeoutMS, bool OptLinger,
    int sqlPort, const char *sqlUser, const char *sqlPwd,
    const char *dbName, int connPoolNum, int threadNum
  );
  ~WebServer();  
  void Start(); // 启动服务器

private:
  static int SedFdNonblock(int fd); // 将io处理更改为非阻塞的方式
  
  bool Init_(); // 服务器初始化
  void InitEventMode_(); // 初始化事件工作模式
  void AddClient_(); // 添加客户端连接
  
  void DealListen_();  // 监听端口 接收新的客户端连接
  void DealWrite_(HttpConn *client); // 添加写事件
  void DealRead_(HttpConn *client); // 添加读事件

  void SendError_(int fd, const char *info); // 发送错误信息至文件描述符
  void ExtentTime_(HttpConn *client); // 重置连接超时时间
  void CloseConn_(HttpConn *client); // 关闭连接

  void OnRead_(HttpConn *client); // 连接读事件 
  void OnWrite_(HttpConn *client); // 连接写事件
  void OnProcess(HttpConn *client); // 检测连接后续处理

private:
  static const int MAX_FD = 65536; // 2^16 文件描述符的最大数量

  int port_; // 服务器端口
  int timeoutMS_; // 链接保留时间
  bool isClose_; // 服务器是否关闭
  int listenFd; // 监听文件描述符
  char *srcDir; // 资源目录相对路径

  uint32_t listenEventMode_; // 监听文件描述符对应的事件工作模式
  uint32_t connEventMode_; // 连接文件描述符对应的事件工作模式

  std::unordered_map<int, HttpConn> users_; // 每个文件描述符对应的连接状态
  std::unique_ptr<HeapTimer> timer_; // 服务器定时器
  std::unique_ptr<ThreadPool> threadpool_; // 线程池
  std::unique_ptr<Epoller> epoller_; // epoll对象
};

#endif // WEBSEVER_H_