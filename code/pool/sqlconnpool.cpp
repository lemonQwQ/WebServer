#include "sqlconnpool.h"
// using namespace std;

SqlConnPool* SqlConnPool::Instance() {
  static SqlConnPool sqlConnPool;
  return &sqlConnPool;
}

MYSQL* SqlConnPool::GETSqlConn() {
  MYSQL *sql = nullptr;
  /* if (connQue.empty()) {
      return nullptr;
  }*/
  sem_wait(&semId_);
  {
    std::lock_guard<std::mutex> locker(mtx_);
    sql = connQue.front();
    connQue.pop();
  }
  return sql;
}

void SqlConnPool::FreeSqlConn(MYSQL **conn) {
  assert(*conn);
  
  std::lock_guard<std::mutex> locker(mtx_);
  connQue.push(*conn);
  *conn = nullptr;
  sem_post(&semId_);
}

int SqlConnPool::GetFreeConnCount() {
  std::lock_guard<std::mutex> locker(mtx_);
  return connQue.size();    
}

  // Init 初始化数据库
void SqlConnPool::Init(const char *host, int post, const char *user,
                       const char *pwd, const char *dbName, int connSize) {
  assert(connSize > 0);
  for (int i = 0; i < connSize; i++) {
      MYSQL *sql = nullptr;
      sql = mysql_init(sql);
      if (!sql) {
        assert(sql);
      }
      sql = mysql_real_connect(sql, host, user, pwd, dbName, post, nullptr, 0);
      if (!sql) {
        assert(sql);
      }
      connQue.push(sql);
  }
  sem_init(&semId_, 0, connSize);
}

void SqlConnPool::ClosePool() {
  
  std::lock_guard<std::mutex> locker(mtx_);
  while(!connQue.empty()) {
    MYSQL *item = connQue.front();
    connQue.pop();
    mysql_close(item);
  }
  
  mysql_library_end();
}