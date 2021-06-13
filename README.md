# WebServer
用c++实现的高性能WEB服务器

# 功能
数据库连接池减少了数据库连接建立销毁时的开销，利用RAII机制对连接池进行管理，防止mysql连接丢失；
封装buffer类实现对IO缓冲区的读写功能，调用readv分散读减少因多次系统调用造成的开销
利用正则表达式与有限状态机解析http请求报文；
引用小根堆实现定时器，实时管理连接的客户端；
