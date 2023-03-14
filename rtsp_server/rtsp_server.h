#ifndef RTSP_SERVER_H_
#define RTSP_SERVER_H_

class RtspServer {
public:
    RtspServer(int server_port);
    ~RtspServer();
public:
    bool StartServerLoop();
private:
    bool CreateServerSocket();
    int AcceptClientSocket();
    void DoRtspCommunication(int client_socket);
    void CloseServer();
    void BuildOptionsMessage(char *buf, int CSeq); 
    void BuildDescribeMessage(char *buf, int CSeq, char *url); 
    void BuildSetupMessage(char *buf, int CSeq, int client_rtp_port, int client_rtcp_port);
    void BuildPlayMessage(char *buf, int CSeq);
private:
    int server_port_;
    int server_rtp_port_;
    int server_rtcp_port_;
    int server_socket_;
};

#endif // RTSP_SERVER_H_
