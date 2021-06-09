#include "buffer.h"

Buffer::Buffer(int initSize = 1024):buff_(initSize), writePos_(0), readPos_(0) {}
    
size_t Buffer::WritableBytes() const {
  return buff_.size() - writePos_;
}

size_t Buffer::ReadableBytes() const {
  return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes() const {
  return readPos_;      
}

const char* Buffer::Peek() const {
  return BeginPtr_() + readPos_;
}

void Buffer::EnsureWriteable(size_t len) {
  if (WritableBytes() < len) {
    MakeSpace(len);
  }
  assert(WritableBytes() >= len);
}
void Buffer::HasWritten(size_t len) {
  writePos_ += len; 
  assert(writePos_ <= buff_.size());
}
  
void Buffer::Retrieve(size_t len) {
  assert(len <= ReadableBytes());
  readPos_ += len;
}

void Buffer::RetrieveAll() {
  if (buff_.size() == 0) return;
  bzero(&buff_[0], buff_.size());
  readPos_ = 0;
  writePos_ = 0;
}

void Buffer::RetrieveUntil(const char *end) {
  assert(Peek() <= end);
  Retrieve(end - Peek());
}

std::string Buffer::RetrieveAllToStr() {
  std::string str(Peek(), ReadableBytes());
  RetrieveAll();
  return str;
}

const char* Buffer::BeginWriteConst() const {
  return BeginPtr_() + writePos_;
}
char* Buffer::BeginWrite() {
  return BeginPtr_() + writePos_;
}  

// 缓冲区中添加数据
void Buffer::Append(const std::string& str) {
  Append(str.data(), str.length());
}

void Buffer::Buffer::Append(const void *data, size_t len) {
  assert(data);
  Append(static_cast<const char*>(data), len);    
}

void Buffer::Append(const char *str, size_t len) {
  assert(str);
  EnsureWriteable(len);
  std::copy(str, str+len, BeginWrite());
  HasWritten(len);
}

void Buffer::Append(const Buffer& buf) {
  Append(buf.Peek(), buf.ReadableBytes());
}


// 封装read函数以及write函数，操作缓冲区读写
ssize_t Buffer::ReadFd(int fd, int *Errno) {
  char tem[65535];
  struct iovec iov[2];
  const size_t writable = WritableBytes();
  
  iov[0].iov_base = BeginPtr_() + writePos_;
  iov[0].iov_len = writable;
  iov[1].iov_base = tem;
  iov[1].iov_len = sizeof(tem);

  const ssize_t len = readv(fd, iov, 2);
  if (len < 0) {
    *Errno = errno;
  } else if (static_cast<size_t>(len) <= writable) {
    HasWritten(len);
  } else {
    HasWritten(writable);
    Append(tem, len - writable);
  }
  return len;
}

ssize_t Buffer::WriteFd(int fd, int *Errno) {
  size_t readSize = ReadableBytes();
  // struct iovec iov[2];
  ssize_t len = write(fd, BeginPtr_() + readPos_, readSize);
  if (len < 0) {
    *Errno = errno;
    return len;
  }
  // readPos_ += len;
  Retrieve(len);
  return len;
}

char* Buffer::BeginPtr_() {
  return &*buff_.begin();
}

const char* Buffer::BeginPtr_() const {
  return &*buff_.begin();
}

void Buffer::MakeSpace(size_t len) {
  if (len > WritableBytes() + PrependableBytes()) {
    buff_.resize(writePos_+len+1);
  } else {
    size_t readable = ReadableBytes();
    std::copy(Peek(), BeginWriteConst(), BeginPtr_());
    writePos_ = readable; 
    readPos_ = 0;
    assert(readable == ReadableBytes());
  }
}
