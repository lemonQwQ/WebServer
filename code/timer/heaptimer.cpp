#include "heaptimer.h"

// newExpires 过期时长
void HeapTimer::adjust(int fd, int newExpires) {
  assert(!heap_.empty() && fd > 0);
  heap_[ref_[fd]].expires = Clock::now() + MS(newExpires);
  siftdown_(ref_[fd], heap_.size());
}

void HeapTimer::add(int fd, int timeOut, const TimeoutCallBack& tcb) {
  auto itor = ref_.find(fd);
  int idx = -1;
  if (itor == ref_.end()) {
    idx = heap_.size();
    ref_[fd] = idx;
    heap_.push_back({fd, Clock::now() + MS(timeOut), tcb});
    siftup_(idx);
  } else {
    idx = itor->second;
    heap_[idx].expires = Clock::now() + MS(timeOut);
    heap_[idx].tcb = tcb;
    siftdown_(idx, heap_.size());
    siftup_(idx);
  }
}

void HeapTimer::doWork(int fd) {
  auto itor = ref_.find(fd);
  if (heap_.empty() || itor == ref_.end()) {
    return;
  }
  size_t idx = itor->second;
  heap_[idx].tcb();
  del_(idx);
}

void HeapTimer::clear() {
  ref_.clear();
  heap_.clear();
}

void HeapTimer::tick() {
  while (!heap_.empty()) {
    TimerNode tnode = heap_.front();
    if (std::chrono::duration_cast<MS>(tnode.expires - Clock::now()).count() > 0) {
        break;
    }
    tnode.tcb();
    pop();
  }
}
  
void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

int HeapTimer::GetNextTick() {
  tick();
  if (heap_.empty()) {
    return -1;
  }
  int t = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
  return t < 0 ? 0: t;
}

void HeapTimer::del_(size_t idx) {
  assert(idx >= 0 && idx < heap_.size());
  size_t i = idx;
  size_t len = heap_.size();
  if (i < len) {
    SwapNode_(idx, len - 1);
    siftdown_(idx, len - 1);
  }
  ref_.erase(heap_.back().fd);
  heap_.pop_back();
}

void HeapTimer::siftup_(size_t idx) {
  assert(idx >= 0 && idx < heap_.size());
  size_t i = idx, j;
  while (i > 0) {
    j = (i - 1) / 2;
    if (heap_[j] < heap_[i]) break;
    SwapNode_(j, i);
    i = j;
  }
}

void HeapTimer::siftdown_(size_t idx, size_t len) {
  assert(idx >= 0 && idx < len);
  assert(len <= heap_.size());
  size_t i = idx, j = i * 2 + 1;
  while (j < len) {
    if (j + 1 < len && heap_[j+1] < heap_[j]) j++;
    if (heap_[i] < heap_[j]) break;  
    SwapNode_(i, j);
    i = j;
    j = 2 * i + 1;
  } 
}
 
void HeapTimer::SwapNode_(size_t lhs, size_t rhs) {
  assert(lhs >= 0 && lhs < heap_.size());
  assert(rhs >= 0 && rhs < heap_.size());
  std::swap(heap_[lhs], heap_[rhs]);
  ref_[heap_[lhs].fd] = lhs;
  ref_[heap_[rhs].fd] = rhs;
}