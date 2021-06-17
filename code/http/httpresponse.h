#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h> 

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
  HttpResponse();
  ~HttpResponse();

  void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
  void MakeResponse(Buffer& buff); // 取出缓冲信息，创建对应响应信息
  void UnmapFile(); // 关闭映射
  char* File();  // 返回 映射地址
  size_t FileLen() const; // 返回映射文件大小
  void ErrorConntent(Buffer& buff, std::string message); // 错误页面
  int Code() const { return code_; }
private:
  void AddStateLine_(Buffer &buff); // 添加响应行
  void AddHeader_(Buffer &buff); // 添加响应头
  void AddContent_(Buffer &buff); // 添加响应体
  
  void ErrorHtml();  // 获取错误响应对应页面
  std::string GetFileType_(); // 获取文件类型

private:
  int code_; // 响应码
  bool isKeepAlive_; // 是否保留连接
  
  std::string path_; // 请求文件相对路径
  std::string srcDir_; // 资源路径

  char *mmFile_; // 映射内存地址
  struct stat mmFileStat_; // 映射文件属性

  static const std::unordered_map<std::string, std::string> SUFFIX_TYPE; // 根据文件后缀确定对应MIME类型
  static const std::unordered_map<int, std::string> CODE_STATUS; // 响应码对应的短语
  static const std::unordered_map<int, std::string> CODE_PATH; // 部分响应码对应的静态文件相对路径 
};

#endif