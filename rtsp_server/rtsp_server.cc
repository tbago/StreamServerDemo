#include "rtsp_server.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"

RtspServer::RtspServer(int server_port) {
    server_port_ = server_port;
    server_rtp_port_ = 55532;
    server_rtcp_port_ = 55533;
}

RtspServer::~RtspServer() {
    CloseServer();
}

bool RtspServer::StartServerLoop() {
    if (!CreateServerSocket()) {
        return false;
    }

    LOGI("Create server socket success. Listen on :%d", server_port_);
    LOGI("Connect address is rtsp://127.0.0.1:%d", server_port_);

    while (true) {
        int client_socket = AcceptClientSocket();
        if (client_socket < 0) {
            return false;
        }

        DoRtspCommunication(client_socket);
    }
 
    return true;
}

bool RtspServer::CreateServerSocket() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        LOGE("Create server socket failed");
        return false;
    }

    int on = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server_port_);

    if (bind(server_socket_, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGE("Bind server port %d failed", server_port_);
        return false;
    }

    if (listen(server_socket_, 5) < 0) {
        LOGE("Listen server failed");
        return false;
    }
   return true;
}

int RtspServer::AcceptClientSocket() {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    socklen_t length = sizeof(addr);
    int client_socket = accept(server_socket_, (struct sockaddr *)&addr, &length);
    if (client_socket < 0) {
        LOGE("Accept client receive failed");
        return -1;
    }
    
    const char *ip_address = inet_ntoa(addr.sin_addr);
    LOGI("Accept connection from %s:%d", ip_address, ntohs(addr.sin_port));
    return client_socket;
}

void RtspServer::DoRtspCommunication(int client_socket) {
    char method[40];
    char url[100];
    char version[40];
    int CSeq = 0;

    int client_rtp_port = 0;
    int client_rtcp_port = 0;

    const int kReceiveBufSize = 2000;
    char receive_buf[kReceiveBufSize];
    while (true) {
        int receive_length = recv(client_socket, receive_buf, kReceiveBufSize, 0);
        if (receive_length <= 0) {
            LOGE("Receive data failed");
            break;
        }
        receive_buf[receive_length] = '\0';

        LOGI("Receive:%s", receive_buf);

        const char *separate = "\n";
        char *line = strtok(receive_buf, separate);
        while (line != nullptr) {
            if (strstr(line, "OPTIONS") ||
                strstr(line, "DESCRIBE") ||
                strstr(line, "SETUP") ||
                strstr(line, "PLAY"))
            {
                int ret = sscanf(line, "%s %s %s\r\n", method, url, version);
                if (ret != 3) {
                    LOGE("Parse method failed, %s", line);
                }
            }
            else if (strstr(line, "CSeq") != nullptr) {
                int ret = sscanf(line, "CSeq:%d\r\n", &CSeq);
                if (ret != 1) {
                    LOGE("Parser CSeq failed,%s", line);
                    break;
                }
            }
            else if (strstr(line, "Transport") != nullptr) {
                int ret = sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &client_rtp_port, &client_rtcp_port);
                if (ret != 2) {
                    LOGE("Prase transport failed, %s", line);
                    break;
                }
            }
            line = strtok(NULL, separate);
        }

        const int kSendBufSize = 2000;
        char send_buf[2000];
        LOGI("method:%s", method);
        if (strcmp(method, "OPTIONS") == 0) {
            BuildOptionsMessage(send_buf, CSeq);
        }
        else if (strcmp(method, "DESCRIBE") == 0) {
            BuildDescribeMessage(send_buf, CSeq, url);
        }
        else if (strcmp(method, "SETUP") == 0) {
            BuildSetupMessage(send_buf, CSeq, client_rtp_port, client_rtcp_port);
        }
        else if (strcmp(method, "PLAY") == 0) {
            BuildPlayMessage(send_buf, CSeq);
        }
        else {
            LOGI("Unsupport method :%s", method);
            break;
        }

        send(client_socket, send_buf, strlen(send_buf), 0);

        if (strcmp(method, "PLAY") == 0) {
            LOGI("Start play rtsp stream");

            while (true) {
                usleep(1000 * 10);
            }
        }
    }  // while(true)
    close(client_socket);
}

void RtspServer::CloseServer() {
    close(server_socket_);
}

void RtspServer::BuildOptionsMessage(char *buf, int CSeq) {
    sprintf(buf, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
        "\r\n",
        CSeq);
}

void RtspServer::BuildDescribeMessage(char *buf, int CSeq, char *url) {
    char sdp_buf[500];
    char local_ip[100];

    sscanf(url, "rtsp://%s[^:]:", local_ip);

    sprintf(sdp_buf, "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=control:track0\r\n",
        time(NULL), local_ip);

    sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
        "Content-Base: %s\r\n"
        "Content-type: application/sdp\r\n"
        "Content-length: %zu\r\n\r\n"
        "%s",
        CSeq,
        url,
        strlen(sdp_buf),
        sdp_buf);
}

void RtspServer::BuildSetupMessage(char *buf, int CSeq, int client_rtp_port, int client_rtcp_port) {
    sprintf(buf, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
        "Session: 66334873\r\n"
        "\r\n",
        CSeq,
        client_rtp_port,
        client_rtcp_port,
        server_rtp_port_,
        server_rtcp_port_);
}

void RtspServer::BuildPlayMessage(char *buf, int CSeq) {
    sprintf(buf, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Range: npt=0.000-\r\n"
        "Session: 66334873; timeout=10\r\n\r\n",
        CSeq);
}
