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
    void LoopReadAndSendVideoFrame();
    void LoopReadAndSendAudioFrame();
    void LoopReadAndSendAVFrameOverTcp();
    void SendH264Frame(RtpPacket *rtp_packet, const char *frame_buf, const int frame_size, bool tcp = false);
    void SendAACFrame(RtpPacket *rtp_packet, const char *frame_buf, const int frame_size, bool tcp = false);
    bool SendRtpPacket(RtpPacket *rtp_packet, int rtp_packet_length, bool tcp = false, int channel = 0);

private:
    bool tcp_mode_;
    int server_port_;
    int server_rtp_port_;
    int server_rtcp_port_;
    int server_socket_;
    int server_rtp_socket_;
    std::string client_ip_;
    int client_rtsp_socket_;
    int client_rtp_port_;
    int client_rtcp_port_;
};

#endif  // RTSP_SERVER_H_
