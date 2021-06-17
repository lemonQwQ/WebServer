#include "./code/server/webserver.h"

int main(){
  WebServer web(
    4000, 3, 60000, false,            /* 端口 ET模式 timeoutMs 优雅退出  */
    3306, "root", "12345678", "webserver",         /* Mysql配置 */
    12, 1, true, 0, 1024 );             /* 连接池数量（数据库） 线程池数量 日志开关 日志等级 日志异步队列容量（0则关闭异步） */        
  web.Start();
  return 0;
}
