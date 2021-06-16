#include "./code/server/webserver.h"

int main(){
  WebServer web(
    4000, 3, 60000, false,            /* 端口 ET模式 timeoutMs 优雅退出  */
    3306, "root", "12345678",         /* Mysql配置 */
    "webserver", 12, 6 );             /* 连接池数量（数据库） 线程池数量 */
               
  web.Start();
  return 0;
}
