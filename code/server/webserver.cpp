#include "webserver.h"

WebServer::WebServer(
  int port, int eventMode, int timeoutMS, bool openLinger, 
  int sqlPort, const char *sqlUser, const char *sqlPwd,
  const char *dbName, int connPoolNum, int threadNum): 
  port_(port), openLinger_(openLinger), timeoutMS_(timeoutMS), isClose_(false), 
  timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller()) {

  srcDir_ = getcwd(nullptr, 256);
  assert(srcDir_);
  strncat(srcDir_, "/resources/", 16);

  HttpConn::srcDir = srcDir_;
  HttpConn::userCount = 0;  

  SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

  InitEventMode_(eventMode);
  if (!InitListen_()) { isClose_ = true; }
}

WebServer::~WebServer() {
  if (listenFd_ != -1) close(listenFd_);
  isClose_ = true;
  free(srcDir_);
  SqlConnPool::Instance()->ClosePool();
}

void WebServer::Start() {
  int timeMS = -1;
  while(!isClose_) {
    if (timeoutMS_ > 0) {
      timeMS = timer_->GetNextTick();
    }
    int eventCnt = epoller_->Wait();
    for (int i = 0; i < eventCnt; i++) {
      int fd = epoller_->GetEventFd(i);
      uint32_t events = epoller_->GetEvents(i);
      auto itor = users_.find(fd);

      if (fd == listenFd_) {
        DealListen_();
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        assert(itor != users_.end());
        CloseConn_(&itor->second);
      } else if (events & EPOLLIN) {
        assert(itor != users_.end());
        DealRead_(&itor->second);
      } else if (events & EPOLLOUT) {
        assert(itor != users_.end());
        DealWrite_(&itor->second);
      }
    }
  }
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0 && fd < MAX_FD);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
  
bool WebServer::InitListen_() {
  if (port_ > 65535 || port_ < 1024) return false;
  
  int ret = -1;

  listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd_ < 0) {
    return false;
  }

  if (openLinger_) {
    struct linger opLinger = { 0 };
    opLinger.l_linger = 1;
    opLinger.l_onoff = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &opLinger, sizeof(opLinger));
    if (ret < 0) {
      // close(listenFd_);
      return false;
    }
  }
  
  int optval = 1;
  ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval));
  if (ret < 0) {
    // close(listenFd_);
    return false;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET; // ipv4
  addr.sin_port = htons(port_);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  ret = bind(listenFd_,  (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    // close(listenFd_);
    return false;
  }

  ret = listen(listenFd_, 8);
  if (ret < 0) {
    // close(listenFd_);
    return false;
  }
  if (!epoller_->AddFd(listenFd_, listenEventMode_ | EPOLLIN)) {
    return false;
  }
  
  SetFdNonblock(listenFd_);  // 设置监听端口为非阻塞
  return true;
}

void WebServer::InitEventMode_(int eventMode) {
  listenEventMode_ = EPOLLRDHUP; // 监听文件描述符事件 对应客户端是否正常关闭
  connEventMode_ = EPOLLONESHOT | EPOLLRDHUP; // 对应文件描述符 最多触发一个读事件、写事件、异常事件
  switch (eventMode)
  {
  case 0: 
    break;
  case 1:
    connEventMode_ |= EPOLLET;
    break;
  case 2:
    listenEventMode_ |= EPOLLET;
    break;  
  default:
    connEventMode_ |= EPOLLET;
    listenEventMode_ |= EPOLLET;
    break;
  }
  HttpConn::isET = (connEventMode_ & EPOLLET);
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
  assert(fd > 0);
  users_[fd].init(fd, addr);
  epoller_->AddFd(fd, EPOLLIN | connEventMode_);
  SetFdNonblock(fd);
  if (timeoutMS_ > 0) {
    timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
  }
}
  
void WebServer::DealListen_() {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);

  do {
    int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
    if (fd <= 0) {
      return;
    } else if (HttpConn::userCount >= MAX_FD) {
      SendError_(fd, "server busy!");
      return;
    }
    AddClient_(fd, addr);
  } while(listenEventMode_ & EPOLLET);
}

void WebServer::DealWrite_(HttpConn *client) {
  assert(client);
  ExtentTime_(client);
  threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::DealRead_(HttpConn *client) {
  assert(client);
  ExtentTime_(client);
  threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::SendError_(int fd, const char *info) {
  assert(fd > 0);
  send(fd, info, strlen(info), 0);
  close(fd);
}

void WebServer::ExtentTime_(HttpConn *client) {
  assert(client);
  if (timeoutMS_ > 0) 
    timer_->adjust(client->GetFd(), timeoutMS_);
}

void WebServer::CloseConn_(HttpConn *client) {
  assert(client);
  epoller_->DelFd(client->GetFd());
  client->Close();
}

void WebServer::OnRead_(HttpConn *client) {
  assert(client);
  int ret = -1;
  int readErrno = 0;
  ret = client->read(&readErrno);
  if (ret <= 0 && readErrno != EAGAIN) {
    CloseConn_(client);
    return;
  }
  OnProcess(client);
}

void WebServer::OnWrite_(HttpConn *client) {
  assert(client);
  int ret = -1;
  int writeErrno = 0;
  ret = client->write(&writeErrno);
  if (client->ToWriteBytes() == 0) {
    if (client->isKeepAlive()) {
      OnProcess(client);
      return;
    }
  } else if (ret < 0) {
    if (writeErrno == EAGAIN) {
      epoller_->ModFd(client->GetFd(), connEventMode_ | EPOLLOUT);
      return;
    }
  }
  CloseConn_(client);
}

void WebServer::OnProcess(HttpConn *client) {
  if (client->process()) {
    epoller_->ModFd(client->GetFd(), connEventMode_ | EPOLLOUT);
  } else {
    epoller_->ModFd(client->GetFd(), connEventMode_ | EPOLLIN);
  }
}
