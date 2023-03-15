#ifndef RTSP_SERVER_H_
#define RTSP_SERVER_H_

#include <string>

struct RtpPacket;

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
    void LoopReadAndSendFrame();
    void SendFrameByUdp(RtpPacket *rtp_packet, const char *frame_buf, const int frame_size);
    bool SendRtpPacket(RtpPacket *rtp_packet, int rtp_packet_length);

private:
    int server_port_;
    int server_rtp_port_;
    int server_rtcp_port_;
    int server_socket_;
    int server_rtp_socket_;
    std::string client_ip_;
    int client_rtp_port_;
    int client_rtcp_port_;
};

#endif  // RTSP_SERVER_H_
