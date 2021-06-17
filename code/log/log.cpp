#include "log.h"

Log* Log::Instance() {
  static Log log;
  return &log;
}

void Log::FlushLogThread() {
  Log::Instance()->AsyncWrite_();
}

void Log::init(int level = 1, const char *path,const char *suffix, int maxQueueCapacity) {
  isOpen_ = true;
  level_ = level;
  if (maxQueueCapacity > 0) {
    isAsync_ = true;
    if (!que_) { 
      std::unique_ptr<BlockQueue<std::string> > newQueue(new BlockQueue<std::string>);
      que_ = move(newQueue);

      std::unique_ptr<std::thread> NewThread(new std::thread(FlushLogThread));
      writeThread_ = move(NewThread);
    }
  } else {
    isAsync_ = false;
  }

  lineCount_ = 0;

  time_t timer = time(nullptr);
  struct tm *sysTime = localtime(&timer);
  struct tm t = *sysTime;

  path_ = path;
  suffix_ = suffix;
  char fileName[LOG_NAME_LEN] = { 0 };
  snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
           path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  toDay_ = t.tm_mday;

  {
    std::lock_guard<std::mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) {
      flush();
      fclose(fp_);
    }

    fp_ = fopen(fileName, "a");
    if (fp_ == nullptr) { // 文件不存在
      mkdir(path_, 0777); 
      fp_ = fopen(fileName, "a");
    }
    assert(fp_ != nullptr);
  }
}

void Log::write(int level, const char *format, ...) {
  struct  timeval now = { 0, 0 };
  gettimeofday(&now, nullptr); // 获取当前时间信息
  time_t tSec = now.tv_sec;
  struct tm *sysTime = localtime(&tSec);
  struct tm t = *sysTime;
  va_list vaList;  

  if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
    char newFile[LOG_NAME_LEN];
    char tail[36] = { 0 };
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    std::unique_lock<std::mutex> locker(mtx_);
    locker.unlock();
    if (toDay_ != t.tm_mday) {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
      locker.lock();
      toDay_ = t.tm_mday;
      lineCount_ = 0;
    } else {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
      locker.lock();
    }

    flush();
    fclose(fp_);
    fp_ = fopen(newFile, "a");
    assert(fp_ != nullptr);
  }

  {
    std::unique_lock<std::mutex> locker(mtx_);
    lineCount_++;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                       t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                       t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    
    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    va_start(vaList, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
    va_end(vaList);

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    if (isAsync_ && que_ && que_->full()) {
      que_->push_back(buff_.RetrieveAllToStr());
    } else {
      fputs(buff_.Peek(), fp_);
    }
    buff_.RetrieveAll();
  }
}

void Log::flush() {
  if (isAsync_) {
    que_->flush();
  }
  fflush(fp_);
}

int Log::GetLevel() {
  std::lock_guard<std::mutex> locker(mtx_);
  return level_;
}

void Log::SetLevel(int level) {
  std::lock_guard<std::mutex> locker(mtx_);
  level_ = level;
}

Log::Log():lineCount_(0), isAsync_(false), writeThread_(nullptr),
           que_(nullptr), toDay_(0), fp_(nullptr) {}

Log::~Log() {
  if (writeThread_ && writeThread_->joinable()) {
    while(!que_->empty()) {
      que_->flush();
    }
    que_->Close();
    writeThread_->join();
  }
  if (fp_) {
    std::lock_guard<std::mutex> locker(mtx_);
    flush();
    fclose(fp_);
  }
}

void Log::AppendLogLevelTitle_(int level) {
  switch (level)
  {
    case 0:
      buff_.Append("[debug]: ", 9);
      break;
    case 1:
      buff_.Append("[info] : ", 9);
      break;
    case 2:
      buff_.Append("[warn] : ", 9);
      break;
    case 3:
      buff_.Append("[error]: ", 9);
      break;
    default:
      buff_.Append("[info] : ", 9);
      break;
  }
}

void Log::AsyncWrite_() {
  std::string str = "";
  while (que_->pop(str)) {
    std::lock_guard<std::mutex> locker(mtx_);
    fputs(str.c_str(), fp_);
  }
}
