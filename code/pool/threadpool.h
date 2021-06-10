#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>
#include <assert.h>

class ThreadPool {
public:
  explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
    assert(threadCount > 0);
    for (size_t i = 0; i < threadCount; i++) {
      std::thread([pool = pool_] {
        std::unique_lock<std::mutex> locker(pool->mtx); // 自旋
        while(true) {
          if (!pool->tasks.empty()) {
            auto task = std::move(pool->tasks.front()); // 右值
            pool->tasks.pop();
            locker.unlock();
            task();
            locker.lock();
          } else if (pool->isClosed) {
            // 线程池已关闭 立即退出
            break;
          } else {
            pool->cond.wait(locker); // 先解锁，等待条件变量
          }
        }
      }).detach(); // 线程分离
    }
  }

  ThreadPool() = default;
  ThreadPool(ThreadPool&&) = default;

  ~ThreadPool() {
    if (static_cast<bool>(pool_)) {
      {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->isClosed = true;  // 连接池关闭
      }
      pool_->cond.notify_all();  // 唤醒所有阻塞信号量
    }
  }

  template<class F>
  void AddTask(F&& task) {
    {
      std::lock_guard<std::mutex> locker(pool_->mtx);
      // tasks.push(task);
      pool_->tasks.emplace(std::forward<F>(task)); // 保存原先的状态。
    }
    pool_->cond.notify_one(); // 随机一个唤醒阻塞信号量
  }

private:
  struct Pool {
    std::mutex mtx;
    std::condition_variable cond;
    bool isClosed;
    std::queue<std::function<void()>> tasks;
  };
  std::shared_ptr<Pool> pool_;
};

#endif // THREADPOOL_H_