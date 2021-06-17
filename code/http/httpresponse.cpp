#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
  { ".html", "text/html" },
  { ".xml", "text/xml" },
  { ".xhtml", "application/xhtml+xml" },
  { ".txt", "text/plain" },
  { ".rtf", "application/rtf" },
  { ".pdf", "application/pdf" },
  { ".word", "application/nsword" },
  { ".png", "image/png" },
  { ".gif", "image/gif"},
  { ".jpg", "image/jpeg"},
  { ".jpeg", "image/jpeg"},
  { ".au",    "audio/basic" },
  { ".mpeg",  "video/mpeg" },
  { ".mpg",   "video/mpeg" },
  { ".avi",   "video/x-msvideo" },
  { ".gz",    "application/x-gzip" },
  { ".tar",   "application/x-tar" },
  { ".css",   "text/css "},
  { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS {
  { 200, "OK" },
  { 400, "Bad Request" },
  { 403, "Forbidden" },
  { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH {
  { 400, "/400.html" },
  { 403, "/403.html" },
  { 404, "/404.html" },
};

HttpResponse::HttpResponse():code_(-1), path_(""), srcDir_(""), isKeepAlive_(false), mmFile_(nullptr), mmFileStat_({ 0 }) {}

HttpResponse::~HttpResponse() {
  UnmapFile();
}

void HttpResponse::Init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code) {
  LOG_DEBUG("httpresponse srcDir: %s path: %s code: %d", srcDir.c_str(), path.c_str(), code);
  srcDir_ = srcDir;
  path_ = path;
  isKeepAlive_ = isKeepAlive;
  code_ = code;
  if (mmFile_) { 
    UnmapFile(); 
  }
  mmFileStat_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer& buff) {
  if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
    code_ = 404;
  } else if (!(mmFileStat_.st_mode & S_IROTH)) {
    code_ = 403;
  } else {
    code_ = 200;
  }
  ErrorHtml();
  AddStateLine_(buff);
  AddHeader_(buff);
  AddContent_(buff);
}

void HttpResponse::UnmapFile() {
  if (mmFile_) {
    munmap(mmFile_, mmFileStat_.st_size);
    mmFile_ = nullptr;
  }
}

char* HttpResponse::File() {
  return mmFile_;
}

size_t HttpResponse::FileLen() const {
  return mmFileStat_.st_size;
}

void HttpResponse::ErrorConntent(Buffer& buff, std::string message) {
  std::string status;
  std::string body;
  auto itor = CODE_STATUS.find(code_);
  if (itor != CODE_STATUS.end()) {
    status = itor->second;
  } else {
    status = "Bad Request";
  }
  body += std::to_string(code_) + " : " + status  + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";

  buff.Append("Content-length: " + std::to_string(body.length()) +  "\r\n\r\n");
  buff.Append(body);
}

void HttpResponse::AddStateLine_(Buffer &buff) {
  std::string status;
  auto itor = CODE_STATUS.find(code_);
  if (itor == CODE_STATUS.end()) {
    status = itor->second;
  } else {
    status = CODE_STATUS.find(400)->second;
  }
  buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer &buff) {
  buff.Append("Connection: ");
  if (isKeepAlive_) {
    buff.Append("keep-alive\r\n");
    buff.Append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.Append("close\r\n");
  }
  buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer &buff) {
  LOG_DEBUG("mark1");
  int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
  if (srcFd < 0) {
    ErrorConntent(buff, "File NotFound!");
    return;
  }
  LOG_DEBUG("mark2");

  LOG_DEBUG("file path %s", (srcDir_ + path_).data());
  int *mmRet = (int *)mmap(nullptr, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
  if (*mmRet == -1) {
    ErrorConntent(buff, "File NotFound!");
    return;
  }
  mmFile_ = (char *)mmRet;
  close(srcFd);
  buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}
  
void HttpResponse::ErrorHtml() {
  auto itor = CODE_PATH.find(code_);
  if (itor != CODE_PATH.end()) {
    path_ = itor->second;
    stat((srcDir_ + path_).data(), &mmFileStat_);
  }
}
  
std::string HttpResponse::GetFileType_() {
  std::string::size_type idx = path_.find_last_of('.');
  if (idx == std::string::npos) {
    return "text/plain";
  }
  auto itor = SUFFIX_TYPE.find(path_.substr(idx));
  if (itor == SUFFIX_TYPE.end()) {
    return "text/plain";
  }
  return itor->second;
}