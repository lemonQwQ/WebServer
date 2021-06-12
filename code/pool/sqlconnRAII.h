#ifndef SQLCONNRAII_H_
#define SQLCONNRAII_H_
#include "sqlconnpool.h"

// raii机制管理线程池，保证资源不会泄露
class SqlConnRAII {
public:
  SqlConnRAII(MYSQL **sqlconn, SqlConnPool *connpool) {
    *sqlconn = connpool->GETSqlConn();    
    sql_ = sqlconn;
    pool_ = connpool;
  }
  ~SqlConnRAII() {
    if (*sql_) {
      pool_->FreeSqlConn(sql_);
    }
  }
private:
  // 禁止在堆上创建
  static void* operator new(size_t);
  static void* operator new[](size_t);

  MYSQL **sql_;
  SqlConnPool *pool_;
};

#endif // SQLCONNRAII_H_