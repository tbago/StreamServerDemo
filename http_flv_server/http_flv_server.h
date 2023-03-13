#ifndef HTTP_FLV_SERVER_H_
#define HTTP_FLV_SERVER_H_

class HttpFlvServer {
public:
    HttpFlvServer(int server_port) : server_port_(server_port) {}
    ~HttpFlvServer() {CloseServer();}
public:
    bool StartServerLoop();
private:
    void CloseServer();
private:
    int server_port_;
    int server_socket_;
};

#endif  // HTTP_FLV_SERVER_H_
