#ifndef BLOCK_QUEUE_H_
#define BLOCK_QUEUE_H_

#include <deque>
#include <mutex>
#include <condition_variable>
#include <assert.h>

template<class T>
class BlockQueue {
public:
  explicit BlockQueue(size_t capacity = 1000);

  ~BlockQueue();

  void clear();

  bool empty();

  bool full();

  void Close();

  size_t size();
  
  size_t capacity();

  T front();
  
  T back();

  void push_back(const T &item);

  void push_front(const T &item);

  bool pop(T &item);

  bool pop(T &item, int timeout);

  void flush();

private:
  std::deque<T> que_;
  std::mutex mtx_;
  std::size_t capacity_;
  bool isClose_;
  std::condition_variable condConsumer_;
  std::condition_variable condProducer_;
};

template<class T>
BlockQueue<T>::BlockQueue(size_t capacity):capacity_(capacity), isClose_(false) {
  assert(capacity_ > 0);
}

template<class T>
BlockQueue<T>::~BlockQueue() {
  Close();
}

template<class T>
void BlockQueue<T>::clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  que_.clear();
}

template<class T>
bool BlockQueue<T>::empty() {
  std::lock_guard<std::mutex> locker(mtx_);
  return que_.empty();
}

template<class T>
bool BlockQueue<T>::full() {
  std::lock_guard<std::mutex> locker(mtx_);
  return que_.size() >= capacity_;
}

template<class T>
void BlockQueue<T>::Close() {
  {
    std::lock_guard<std::mutex> locker(mtx_);
    que_.clear();
    isClose_ = true;
  }
  condProducer_.notify_all();
  condConsumer_.notify_all();
}

template<class T>
size_t BlockQueue<T>::size() {
  std::lock_guard<std::mutex> locker(mtx_);
  return que_.size();
}

template<class T>
size_t BlockQueue<T>::capacity() {
  return capacity_;
}

template<class T>
T BlockQueue<T>::front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return que_.front();
}

template<class T>
T BlockQueue<T>::back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return que_.back();
}

template<class T>
void BlockQueue<T>::push_back(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (que_.size() >= capacity_)
  {
    condProducter_.wait(locker);
  }
  que_.push_back(item);
  condConsumer_.notify_one();
}

template<class T>
void BlockQueue<T>::push_front(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (que_.size() >= capacity_)
  {
    condProducter_.wait(locker);
  }
  que_.push_front(item);
  condConsumer_.notify_one();  
}

template<class T>
bool BlockQueue<T>::pop(T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while(que_.empty()) {
    condConsumer_.wait(locker);
    if (isClose_) {
      return false;
    }
  }
  
  item = que_.front();
  que.pop_front();
  condProducer_.notify_one();
  return true;
}

template<class T>
bool BlockQueue<T>::pop(T &item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx_);
  while(que_.empty()) {
    if (condConsumer_.wait_for(locker, timeout)
        == std::cv_status::timeout) {
      return false;
    }
    if (isClose_) {
      return false;
    }
  }
  item = que_.front();
  que_.pop_front();
  condProducer_.notify_one();
  return true;
}

template<class T>
void BlockQueue<T>::flush() {
  condConsumer_.notify_one();
}

#endif // BLOCK_QUEUE_H_