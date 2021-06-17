#ifndef HEAP_TIMER_H_
#define HEAP_TIMER_H_

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
  int fd;
  TimeStamp expires;
  TimeoutCallBack tcb;
  bool operator<(const TimerNode& tn) const {
      return expires < tn.expires;
  }
};

class HeapTimer {
public:
  HeapTimer() { heap_.reserve(64); }
  ~HeapTimer() { clear(); }

  void adjust(int fd, int newExpires); // 调整结点

  void add(int fd, int timeOut, const TimeoutCallBack& tcb);  // 添加 \ 更改一个结点

  void doWork(int fd); // 调用回调函数

  void clear(); // 清除所有结点

  void tick(); // 清除所有过期结点
  
  void pop(); // 取出最小值

  int GetNextTick(); // 取出下一个将要过期的结点

private:
  void del_(size_t idx);  // 删除指定结点

  void siftup_(size_t idx);  // 向上调整

  void siftdown_(size_t idx, size_t len); // 向下调整
 
  void SwapNode_(size_t lhs, size_t rhs); // 交换结点
private:
  std::vector<TimerNode> heap_; // 堆数组
  std::unordered_map<int, size_t> ref_; // fd对应在堆数组的位置
};

#endif // HEAP_TIMER_H_