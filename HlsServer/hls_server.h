#ifndef HLS_SERVER_H_
#define HLS_SERVER_H_

class HlsServer final {
public:
    HlsServer(int server_port):server_port_(server_port) {}
    ~HlsServer() {}
public:
    bool StartServerLoop();
private:
    int CreateServerSocket();
    void CloseServer();
    int AcceptClientSocket();
    int CreateM3u8(const char * m3u8_filename);
    int AppendM3u8(const char *m3u8_filename, const char *ts_filename);
    bool ReceiveData(int socket);
private:
    int server_port_;
    int server_socket_;
}; //HlsServer

#endif // HLS_SERVER_H_
