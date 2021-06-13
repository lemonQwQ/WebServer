
#include "httpconn.h"

bool HttpConn::isET; // 连接以ET模式处理?
const char* HttpConn::srcDir; // 资源目录
std::atomic<int> HttpConn::userCount; // 当前客户端连接数

HttpConn::HttpConn() {
  isClose_ = true;
  addr_ = { 0 };
  fd_ = -1;
}

HttpConn::~HttpConn() {
  Close();
}

void HttpConn::init(int sockFd, const sockaddr_in& addr) {
  assert(sockFd > 0);
  userCount++;
  fd_ = sockFd;
  addr_ = addr;
  
  writeBuff_.RetrieveAll();
  readBuff_.RetrieveAll();
  isClose_ = false;
}

ssize_t HttpConn::read(int *saveErrno) {
  ssize_t len = -1;
  do {
    len = readBuff_.ReadFd(fd_, saveErrno);
    if (len <= 0) break;
  } while(isET);
  return len; // ~
}

ssize_t HttpConn::write(int *saveErrno) {
  ssize_t len = -1;
  do {
    // len = writeBuff_.WriteFd(fd_, saveErrno);
    // if (len <= 0) break;
    len = writev(fd_, iov_, iovCnt_);
    if (len <= 0) {
      *saveErrno = errno;
      break;
    }
    if (iov_[0].iov_len + iov_[1].iov_len == 0) { break; }
    else if (static_cast<size_t>(len) <= iov_[0].iov_len) {
      iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
      iov_[0].iov_len -= len;
      writeBuff_.Retrieve(len);
    } else {
      iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
      iov_[1].iov_len -= (len - iov_[0].iov_len);
      writeBuff_.RetrieveAll();
      iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + iov_[0].iov_len;
      iov_[0].iov_len = 0;
    }
  } while(isET || ToWriteBytes() > 10240);
  return len;
}

void HttpConn::Close() {
  response_.UnmapFile();
  if (isClose_ == false) {
    isClose_ = true;
    userCount--;
    close(fd_);
  }
}

int HttpConn::GetFd() const {
  return fd_;
}

int HttpConn::GetPort() const {
  return ntohs(addr_.sin_port);
}

const char* HttpConn::GetIP() const {
  return inet_ntoa(addr_.sin_addr);
}

sockaddr_in HttpConn::GetAddr() const {
  return addr_;
}

// 返回值： false代表要进行读操作， true代表写操作
bool HttpConn::process() {
  if (readBuff_.ReadableBytes() <= 0) {
    return false;
  } else if (request_.parse(readBuff_)) {
    response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
  } else {
    response_.Init(srcDir, request_.path(), false, 400);
  }

  response_.MakeResponse(writeBuff_);

  iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
  iov_[0].iov_len = writeBuff_.WritableBytes();
  iovCnt_ = 1;

  if (response_.FileLen() > 0 && response_.File()) {
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    iovCnt_ = 2;
  }
  return true;
}

