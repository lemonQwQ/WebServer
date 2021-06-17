#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
  // 解析状态
  enum PARSE_STATE {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH,
  };

  // 常见响应码
  enum HTTP_CODE {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
  };
  HttpRequest();
  ~HttpRequest() = default;

  // void Init();
  bool parse(Buffer& buff);

  std::string path() const;  // 获取请求路径 uri
  std::string& path();  
  std::string method() const; // 获取请求方法
  std::string version() const;  // 获取http版本
  // std::string GetPost(const std::string& key) const;  
  std::string GetPost(const char *key) const; // 在请求头中获取key对应值
  // std::string GetHeader(const std::string& key);
  std::string GetHeader(const char *key) const; // 在请求头中获取key对应值

  bool IsKeepAlive() const; // 获取 是否保持连接状态

private:
  bool ParseRequestLine_(const std::string& line); // 解析请求行
  void ParseHeader_(const std::string& line);  // 解析请求头
  void ParseBody_(const std::string& line); // 解析请求体

  void ParsePath_(); // 解析uri路径 保存相对路径path_中
  void ParsePost_(); // 解析POST请求方法中的请求体
  void ParseFromUrlencoded_(); // 解析表单信息

  // 更新请求用户状态
  static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);
  
  // 0~f 字符转为 0~15
  static int ConverHex(char ch);

  PARSE_STATE state_;
  std::string method_, path_, version_, body_;
  std::unordered_map<std::string, std::string>  header_;
  std::unordered_map<std::string, std::string>  post_;

  static const std::unordered_set<std::string> DEFAULT_HTML;
  static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif