#ifndef LOG_H_
#define LOG_H_

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>

#include "blockqueue.h"
#include "../buffer/buffer.h"


class Log {
public:
  static Log* Instance();
  static void FlushLogThread();

  void init(int level, const char *path = "./log",
            const char *suffix = ".log", 
            int maxQueueCapacity = 1024);
  void write(int level, const char *format, ...);
  void flush(); // 刷新消息队列

  int GetLevel(); // 获取日志等
  void SetLevel(int level); // 设置日志等级
  bool IsOpen() { return isOpen_; } //是否打开日志

private:
  Log();
  ~Log();
  void AppendLogLevelTitle_(int level); // 添加日志标题
  void AsyncWrite_(); //异步输出

private:
  static const int LOG_PATH_LEN = 256;
  static const int LOG_NAME_LEN = 256;
  static const int MAX_LINES = 50000;

  const char *path_;  //日志路径
  const char *suffix_; // 后缀

  int maxLines_;  //日志最多行数
  
  int lineCount_;  //日志当前行数
  int toDay_;  //当前日期
  
  bool isOpen_;  //是否打开日志
  
  Buffer buff_;  //日志缓冲区
  int level_;  // 当前等级
  bool isAsync_;  // 是否异步

  FILE *fp_;  //日志文件
  std::unique_ptr<BlockQueue<std::string>> que_; //阻塞队列 线程安全的队列 异步使用
  std::unique_ptr<std::thread> writeThread_; //日志专用线程 异步使用
  std::mutex mtx_;
};

#define LOG_BASE(level, format, ...)\
  do {\
    Log *log = Log::Instance(); \
    if (log->IsOpen() && log->GetLevel() <= level) { \
      log->write(level, format, ##__VA_ARGS__); \
      log->flush(); \
    }\
  } while(0);

#define   LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define   LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define   LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define   LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif // LOG_H_