
#ifndef BUFFER_H_
#define BUFFER_H_
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <assert.h>

/*
 * Buffer类：标准库封装char实现一个自动增长的缓冲区
 * */
class Buffer {
public:
  Buffer(int initSize = 1024);
  ~Buffer() = default;
    
  size_t WritableBytes() const;  // 获取当前可写缓冲区
  size_t ReadableBytes() const;  // 获取当前可读缓冲区
  size_t PrependableBytes() const; // 获取已释放缓冲区

  const char* Peek() const; // 获取缓冲区首部
  void EnsureWriteable(size_t len); // 配置足够的缓冲区
  void HasWritten(size_t len);  // 增加已写入缓冲区
  
  void Retrieve(size_t len);  // 释放已读出缓冲区
  void RetrieveUntil(const char *end); // 释放缓冲区直到end
  void RetrieveAll();  // 释放全部缓冲区
  std::string RetrieveAllToStr();  // 将缓冲区内容返回，并释放掉全部内容

  const char* BeginWriteConst() const; // 获取可用缓冲区尾部，即可写缓冲首部
  char* BeginWrite();  // 同上

  // 缓冲区中添加数据
  void Append(const std::string& str);
  void Append(const char *str, size_t len);
  void Append(const void *data, size_t len);
  void Append(const Buffer& buf);

  // 封装read函数以及write函数，操作缓冲区读写
  size_t ReadFd(int fd, int *Errno);
  size_t WriteFd(int fd, int *Errno);
private:
  char* BeginPtr_(); // 缓冲区首部
  const char* BeginPtr_() const;
  void MakeSpace(size_t len); // 配置足够的缓冲缓冲区

  std::vector<char> buff_;
  std::atomic<std::size_t> readPos_;
  std::atomic<std::size_t> writePos_;
};

#endif // BUFFER_H
