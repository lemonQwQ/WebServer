#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture"
};
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0},
    {"/login.html", 1}
};


HttpRequest::HttpRequest() {
  method_ = path_ = version_ = body_ = "";
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();    
}

bool HttpRequest::parse(Buffer& buff) {
  const char CRLF[] = "\r\n";
  if (buff.ReadableBytes() <= 0) {
    return false;
  }
  while(buff.ReadableBytes() && state_ != FINISH) {
    const char* lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
    std::string line(buff.Peek(), lineEnd);
    switch (state_)
    {
    case REQUEST_LINE:
      if (!ParseRequestLine_(line)) {
        return false;
      }
      ParsePath_();
      break;
    case HEADERS:
      ParseHeader_(line);
      if (buff.ReadableBytes() <= 2) {  // 请求中没有请求体
        state_ = FINISH;
      } 
      break;
    case BODY:
      ParseBody_(line);
      break;    
    default:
      break;
    }
  }
}

bool HttpRequest::ParseRequestLine_(const std::string& line) {
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch subMatch;
  if (regex_match(line, subMatch, patten)) {
    method_ = subMatch[1];
    path_ = subMatch[2];
    version_ = subMatch[3];
    state_ = HEADERS;
    return true; 
  }
  return false;
}

void HttpRequest::ParsePath_() {
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    if (DEFAULT_HTML.find(path_) != DEFAULT_HTML.end()) {
    path_ += ".html";
    }
  }
}

void HttpRequest::ParseHeader_(const std::string& line) {
  std::regex patten("^([^:]*): (.*)$");
  std::smatch subMatch; 
  if (regex_match(line, subMatch, patten)) {
    header_[subMatch[1]] = subMatch[2];
  } else {
    state_ = BODY;
  }
}

void HttpRequest::ParseBody_(const std::string& line) {
  body_ = line;
  ParsePost_();
  state_ = FINISH;  
}

void HttpRequest::ParsePost_() {
  if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParseFromUrlencoded_();
    auto itor = DEFAULT_HTML_TAG.find(path_);
    if (itor != DEFAULT_HTML_TAG.end()) {
      int tag = itor->second;
      if (UserVerify(post_["username"], post_["password"], tag)) {
        path_ = "/welcome.html";
      } else {
        path_ = "/error.html";
      }
    }
  }
}

void HttpRequest::ParseFromUrlencoded_() {
  std::string key;
  int pre = 0;
  int len = body_.length();
  for (int i = 0; i < len; i++) {
    switch(body_[i]) {
    case '&':
      post_[key] = body_.substr(pre, i - pre);
      pre = i+1;
      break;  
    case '=':
      key = body_.substr(pre, i - pre);
      pre = i+1;
      break;  
    case '+':
      body_[i] = ' ';
      break;  
    case '%':
      int num = ConverHex(body_[i+1]) * 16 + ConverHex(body_[i+2]);
      body_[i+1] = num / 10 + '0';
      body_[i+2] = num % 10 + '0';
      i += 2;
      break;  
    default:
      break;
    }    
  }
  if (len <= pre) return;
  post_[key] = body_.substr(pre, len - pre); 
}

// 更新请求用户状态
bool HttpRequest::UserVerify(const std::string& name, const std::string& pwd, bool isLogin) {
  if (name == "" || pwd == "") return false;
  MYSQL *sql;
  char order[256] = { 0 };  
  bool flag = false;
  MYSQL_RES *res = nullptr;

  SqlConnRAII scr(&sql, SqlConnPool::Instance());
  assert(sql);
  
  snprintf(order, 256, "SELECT password FROM user WHERE username = '%s' LIMIT 1", name.c_str());

  if (mysql_query(sql, order)) {  // 执行指定为一个空结尾的字符串的SQL查询
    return false;
  }

  res = mysql_store_result(sql); // 检索一个完整的结果集
  if (mysql_num_fields(res) > 0) { // 结果集 行数
    while (MYSQL_ROW row = mysql_fetch_row(res)) { // 获取结果集的下一行
      if (isLogin && pwd == row[0]) {
        flag = true;
      } 
    }
    mysql_free_result(res); // 释放结果集
    return flag;
  }
  mysql_free_result(res); // 释放结果集

  if (!isLogin) {
    bzero(order, 256);
    snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
    if (mysql_query(sql, order) == 0) {
      return true;
    }
  }  
  return false;
}


std::string HttpRequest::path() const {
  return path_;
}

std::string& HttpRequest::path() {
  return path_;
} 

std::string HttpRequest::method() const {
  return method_;
}

std::string HttpRequest::version() const {
  return version_;
}

/*std::string HttpRequest::GetPost(const std::string& key) const {
  assert(key != "");
  return GetPost(key.c_str());
}*/

std::string HttpRequest::GetPost(const char *key) const {
  assert(key != nullptr);
  auto itor = post_.find(key);
  if (itor != post_.end()) {
    return itor->second;
  }
  return ""; 
}

/*std::string HttpRequest::GetHeader(const std::string& key) const {
  assert(key != "");
  return GetHeader(key.c_str());
}*/

std::string HttpRequest::GetHeader(const char *key) const {
  assert(key != nullptr);
  auto itor = header_.find(key);
  if (itor != header_.end()) {
    return itor->second;
  }
  return "";
}

bool HttpRequest::IsKeepAlive() const {
  std::string connection = GetHeader("Connection");
  return (connection == "keep-alive" && version_ == "1.1");
}

static int ConverHex(char ch) {
  if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'z') return ch - 'a' + 10;
  if (ch >= '0' && ch <= '9') return ch - '0';
  return ch;
}
