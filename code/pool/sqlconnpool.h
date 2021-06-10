#ifndef SQLCONNPOOL_H_
#define SQLCONNPOOL_H_

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <assert.h>
#include <thread>

class SqlConnPool {
public:
  static SqlConnPool* Instance(); // 单列模式

  MYSQL* GETSqlConn(); // 获取一个sql连接
  void FreeSqlConn(MYSQL *conn); // 释放一个sql连接
  int GetFreeConnCount(); // 获取空闲连接数

  // Init 初始化数据库
  void Init(const char *host, int post, const char *user, 
            const char *pwd, const char *dbName, int connSize = 10); 
  void ClosePool(); // 关闭连接池

private:
  SqlConnPool() = default;
  ~SqlConnPool() = default;

//   int MAXN_CONN_;  // 最大连接数
//   int useCount_;   // 当前占用连接数
//   int freeCount_;  // 空闲连接数

  std::queue<MYSQL *> connQue; // 连接池
  std::mutex mtx_; // 互斥锁
  sem_t semId_;  // 信号量
};

#endif // SQLCONNPOOL_H_