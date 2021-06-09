#ifndef SQLCONNPOOL_H_
#define SQLCONNPOOL_H_

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>

class SqlConnPool {
public:
  static SqlConnPool* Instance(); // 单列模式

private:
  SqlConnPool();
  ~SqlConnPool();

  int MAXN_CONN_;  // 最大连接数
  int useCount_;   // 当前占用连接数
  int freeCount_;  // 空闲连接数

  std::queue<MYSQL *> connQue; // 连接池
  std::mutex mtx_; // 互斥锁
  sem_t semId_;  // 信号量
};

#endif // SQLCONNPOOL_H_