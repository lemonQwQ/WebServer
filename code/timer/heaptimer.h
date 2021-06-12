#ifndef HEAP_TIMER_H_
#define HEAP_TIMER_H_

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <functional>
#include <assert.h>
#include <chrono>


typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
  int id;
  TimeStamp expires;
  TimeoutCallBack tcb;
  bool operator<(const TimerNode& tn) const {
      return expires < tn.expires;
  }
};

class HeapTimer {
public:
  HeapTimer() { heap_.resize(64); }
  ~HeapTimer() { clear(); }

  void adjust(int id, int newExpires); // 调整结点

  void add(int id, int timeOut, const TimeoutCallBack& cb);  // 添加 \ 更改一个结点

  void doWork(int id); // 调用回调函数

  void clear(); // 清除所有结点

  void tick(); // 清除所有过期结点
  
  void pop(); // 取出最小值

  int GetNextTick(); // 取出下一个将要过期的结点

private:
  void del(size_t idx);  // 删除指定结点

  void siftup_(size_t idx);  // 向上调整

  void siftdown(size_t idx, size_t len); // 向下调整
 
  void SwapNode(size_t lhs, size_t rhs); // 交换结点
private:
  std::vector<TimerNode> heap_; // 堆数组
  std::unordered_map<int, size_t> ref_;
};

#endif // HEAP_TIMER_H_