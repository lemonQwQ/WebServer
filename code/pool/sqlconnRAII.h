#ifndef SQLCONNRAII_H_
#define SQLCONNRAII_H_
#include "sqlconnpool.h"

// raii机制管理线程池，保证资源不会泄露
class SqlConnRAII {
public:
  SqlConnRAII(MYSQL **sqlconn, SqlConnPool *connpool) {
    sql_ = connpool->GETSqlConn();    
    *sqlconn = sql_;
    pool_ = connpool;
  }
  ~SqlConnRAII() {
      if (sql_) {
          pool_->FreeSqlConn(&sql_);
      }
  }
private:
  MYSQL *sql_;
  SqlConnPool *pool_;
};

#endif // SQLCONNRAII_H_