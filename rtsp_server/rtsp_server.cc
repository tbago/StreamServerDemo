#include "rtsp_server.h"

#include <arpa/inet.h>
#include <cassert>
#include <cmath>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "adts_header.h"
#include "log.h"
#include "rtp.h"

static inline bool H264FourByteStartCode(const char *buf);
static inline bool H264ThreeByteStartCode(const char *buf);
static const char *H264FindStartCode(const char *buf, int buf_length);
static int H264GetFrameFromFile(FILE *fp, char *frame_buf, const int frame_buf_size);

static int CreateUdpSocket();
static bool BindSocketAddress(int socket_fd, const char *ip_address, int port);

static bool SendDataByUdp(int socket, const char *ip_address, const int port, const char *data, const int data_size);

static const char *kH264TestFile = "./data/test.h264";
static const float kFrameRate = 29.97;
static const int kRtmpVideoTimeBase = 90000;

static const char *kAACTestFile = "./data/test.aac";
static const int kAudioSampleRate = 44100;
static const int kAudioChannel = 2;

static std::mutex send_mutex;

RtspServer::RtspServer(int server_port) {
    server_port_ = server_port;
    server_rtp_port_ = 55532;
    server_rtcp_port_ = 55533;
}

RtspServer::~RtspServer() { CloseServer(); }

bool RtspServer::StartServerLoop() {
    if (!CreateServerSocket()) {
        return false;
    }

    LOGI("Create server socket success. Listen on :%d", server_port_);
    LOGI("Connect address is rtsp://127.0.0.1:%d", server_port_);

    tcp_mode_ = true;

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
    client_ip_ = ip_address;
    return client_socket;
}

void RtspServer::DoRtspCommunication(int client_socket) {
    char method[40];
    char url[100];
    char version[40];
    int CSeq = 0;

    int client_rtp_port = 0;
    int client_rtcp_port = 0;

    client_rtsp_socket_ = client_socket;

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
            if (strstr(line, "OPTIONS") || strstr(line, "DESCRIBE") || strstr(line, "SETUP") || strstr(line, "PLAY")) {
                int ret = sscanf(line, "%s %s %s\r\n", method, url, version);
                if (ret != 3) {
                    LOGE("Parse method failed, %s", line);
                }
            } else if (strstr(line, "CSeq") != nullptr) {
                int ret = sscanf(line, "CSeq:%d\r\n", &CSeq);
                if (ret != 1) {
                    LOGE("Parser CSeq failed,%s", line);
                    break;
                }
            } else if (strstr(line, "Transport") != nullptr) {
                if (tcp_mode_) {
                    if (sscanf(line, "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n") != 0) {
                        LOGE("Parse transport failed, %s", line);
                        break;
                    }
                } else {
                    int ret = sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &client_rtp_port_,
                                     &client_rtcp_port_);
                    if (ret != 2) {
                        LOGE("Prase transport failed, %s", line);
                        break;
                    }
                }
            }
            line = strtok(NULL, separate);
        }

        const int kSendBufSize = 2000;
        char send_buf[kSendBufSize];
        LOGI("method:%s", method);
        if (strcmp(method, "OPTIONS") == 0) {
            BuildOptionsMessage(send_buf, CSeq);
        } else if (strcmp(method, "DESCRIBE") == 0) {
            BuildDescribeMessage(send_buf, CSeq, url);
        } else if (strcmp(method, "SETUP") == 0) {
            BuildSetupMessage(send_buf, CSeq, client_rtp_port, client_rtcp_port);

            if (!tcp_mode_) {
                // create rtp socket
                if (server_rtp_socket_ = CreateUdpSocket(); server_rtp_socket_ < 0) {
                    LOGE("Create rtp socket failed");
                    break;
                }

                if (!BindSocketAddress(server_rtp_socket_, "0.0.0.0", server_rtp_port_)) {
                    LOGE("Bind rtp socket %d failed", server_rtp_port_);
                    break;
                }
            }
        } else if (strcmp(method, "PLAY") == 0) {
            BuildPlayMessage(send_buf, CSeq);
        } else {
            LOGI("Unsupport method :%s", method);
            break;
        }

        send(client_socket, send_buf, strlen(send_buf), 0);

        if (strcmp(method, "PLAY") == 0) {
            LOGI("Start play rtsp stream");

            if (!tcp_mode_) {
                // LoopReadAndSendVideoFrame();
                LoopReadAndSendAudioFrame();
            } else {
                LoopReadAndSendAVFrameOverTcp();
            }
        }
    }  // while(true)

    close(client_socket);
    close(server_rtp_socket_);
}

void RtspServer::CloseServer() { close(server_socket_); }

void RtspServer::BuildOptionsMessage(char *buf, int CSeq) {
    sprintf(buf,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
            "\r\n",
            CSeq);
}

void RtspServer::BuildDescribeMessage(char *buf, int CSeq, char *url) {
    char sdp_buf[500];
    char local_ip[100];

    sscanf(url, "rtsp://%s[^:]:", local_ip);
    if (!tcp_mode_) {
        // rtsp with video only
        // sprintf(sdp_buf,
        //         "v=0\r\n"
        //         "o=- 9%ld 1 IN IP4 %s\r\n"
        //         "t=0 0\r\n"
        //         "a=control:*\r\n"
        //         "m=video 0 RTP/AVP 96\r\n"
        //         "a=rtpmap:96 H264/%d\r\n"
        //         "a=control:track0\r\n",
        //         time(NULL), local_ip, kRtmpVideoTimeBase);

        // rtsp with audio only
        sprintf(sdp_buf,
                "v=0\r\n"
                "o=- 9%ld 1 IN IP4 %s\r\n"
                "t=0 0\r\n"
                "a=control:*\r\n"
                "m=audio 0 RTP/AVP 97\r\n"
                "a=rtpmap:97 mpeg4-generic/%d/%d\r\n"
                "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n"
                "a=control:track0\r\n",
                time(NULL), local_ip, kAudioSampleRate, kAudioChannel);
    } else {
        // rtp video and audio stream over tcp
        sprintf(sdp_buf,
                "v=0\r\n"
                "o=- 9%ld 1 IN IP4 %s\r\n"
                "t=0 0\r\n"
                "a=control:*\r\n"
                "m=video 0 RTP/AVP/TCP 96\r\n"
                "a=rtpmap:96 H264/%d\r\n"
                "a=control:track0\r\n"
                "m=audio 1 RTP/AVP/TCP 97\r\n"
                "a=rtpmap:97 mpeg4-generic/%d/%d\r\n"
                "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n"
                "a=control:track1\r\n",
                time(NULL), local_ip, kRtmpVideoTimeBase, kAudioSampleRate, kAudioChannel);
    }
    sprintf(buf,
            "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
            "Content-Base: %s\r\n"
            "Content-type: application/sdp\r\n"
            "Content-length: %zu\r\n\r\n"
            "%s",
            CSeq, url, strlen(sdp_buf), sdp_buf);
}

void RtspServer::BuildSetupMessage(char *buf, int CSeq, int client_rtp_port, int client_rtcp_port) {
    if (tcp_mode_) {
        if (CSeq == 3) {
            sprintf(buf,
                    "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
                    "Session: 66334873\r\n"
                    "\r\n",
                    CSeq);
        } else if (CSeq == 4) {
            sprintf(buf,
                    "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n"
                    "Session: 66334873\r\n"
                    "\r\n",
                    CSeq);
        }
    } else {
        sprintf(buf,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %d\r\n"
                "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                "Session: 66334873\r\n"
                "\r\n",
                CSeq, client_rtp_port, client_rtcp_port, server_rtp_port_, server_rtcp_port_);
    }
}

void RtspServer::BuildPlayMessage(char *buf, int CSeq) {
    sprintf(buf,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Range: npt=0.000-\r\n"
            "Session: 66334873; timeout=10\r\n\r\n",
            CSeq);
}

void RtspServer::LoopReadAndSendVideoFrame() {
    FILE *fp = fopen(kH264TestFile, "rb");
    if (fp == nullptr) {
        LOGE("Cannot open h264 test file");
        return;
    }

    const int frame_buf_size = 500000;  // just for test
    char *frame_buf = (char *)malloc(frame_buf_size);

    RtpPacket *rtp_packet = (RtpPacket *)malloc(frame_buf_size);
    memset(&rtp_packet->header, 0, sizeof(RtpHeader));
    rtp_packet->header.version = RTP_VERSION;
    rtp_packet->header.payload_type = RTP_PAYLOAD_TYPE_H264;
    rtp_packet->header.ssrc = 0x88923423;

    while (true) {
        int frame_size = H264GetFrameFromFile(fp, frame_buf, frame_buf_size);
        if (frame_size < 0) {
            LOGI("Read end of H264 file");
            break;
        }

        SendH264Frame(rtp_packet, frame_buf, frame_size);
        usleep(1000 * 1000 / kFrameRate);
    }
    // release resources
    free(frame_buf);
    free(rtp_packet);
    fclose(fp);
}

void RtspServer::LoopReadAndSendAudioFrame() {
    FILE *fp = fopen(kAACTestFile, "rb");
    if (fp == nullptr) {
        LOGE("Cannot open aac test file");
        return;
    }

    AdtsHeader adts_header;
    static const int kMemorySize = 5000;
    char *frame_buf = (char *)malloc(kMemorySize);
    RtpPacket *rtp_packet = (RtpPacket *)malloc(kMemorySize);
    memset(&rtp_packet->header, 0, sizeof(RtpHeader));
    rtp_packet->header.version = RTP_VERSION;
    rtp_packet->header.payload_type = RTP_PAYLOAD_TYPE_AAC;
    rtp_packet->header.ssrc = 0x32411;

    const int kAdtsHeaderSize = 7;
    while (true) {
        int ret = fread(frame_buf, 1, kAdtsHeaderSize, fp);
        if (ret <= 0) {
            LOGI("Read end of file");
            break;
        }

        if (!ParseAdtsHeader((uint8_t *)frame_buf, ret, &adts_header)) {
            LOGI("Parse adts header failed");
            break;
        }
        ret = fread(frame_buf, 1, adts_header.aac_frame_length - kAdtsHeaderSize, fp);
        if (ret <= 0) {
            LOGE("Read end of file without enough data");
            break;
        }

        SendAACFrame(rtp_packet, frame_buf, ret);

        usleep(2000);
    }
    free(frame_buf);
    free(rtp_packet);
    fclose(fp);
}

void RtspServer::LoopReadAndSendAVFrameOverTcp() {
    std::thread video_thread([&]() {
        std::unique_ptr<FILE, decltype(fclose) *> video_fp(fopen(kH264TestFile, "rb"), fclose);
        if (video_fp == nullptr) {
            LOGE("Cannot open video file %s ", kH264TestFile);
            return;
        }
        const int frame_buf_size = 500000;
        std::unique_ptr<char, decltype(free) *> frame_buf(reinterpret_cast<char *>(malloc(frame_buf_size)),
                                                          free);

        std::unique_ptr<RtpPacket, decltype(free) *> rtp_packet(reinterpret_cast<RtpPacket *>(malloc(frame_buf_size)),
                                                                free);
        memset(&rtp_packet->header, 0, sizeof(RtpHeader));
        rtp_packet->header.version = RTP_VERSION;
        rtp_packet->header.payload_type = RTP_PAYLOAD_TYPE_H264;
        rtp_packet->header.ssrc = 0x88923423;

        LOGI("Start video paly thread");

        while (true) {
            int frame_size = H264GetFrameFromFile(video_fp.get(), frame_buf.get(), frame_buf_size);
            if (frame_size <= 0) {
                LOGI("Read end of H264 file");
                break;
            }
            SendH264Frame(rtp_packet.get(), frame_buf.get(), frame_size, true);
            usleep(1000 * 1000 / kFrameRate);
        }
    });

    std::thread audio_thread([&]() {
        std::unique_ptr<FILE, decltype(fclose) *> audio_fp(fopen(kAACTestFile, "rb"), fclose);
        if (audio_fp == nullptr) {
            LOGE("Cannot open audio file %s", kAACTestFile);
        }
        const int frame_buf_size = 5000;

        std::unique_ptr<char, decltype(free) *> frame_buf(reinterpret_cast<char *>(malloc(frame_buf_size)),
                                                          free);

        std::unique_ptr<RtpPacket, decltype(free) *> rtp_packet(reinterpret_cast<RtpPacket *>(malloc(frame_buf_size)),
                                                                free);

        memset(&rtp_packet->header, 0, sizeof(RtpHeader));
        rtp_packet->header.version = RTP_VERSION;
        rtp_packet->header.payload_type = RTP_PAYLOAD_TYPE_AAC;
        rtp_packet->header.ssrc = 0x32411;

        const int kAdtsHeaderSize = 7;
        AdtsHeader adts_header;
        while (true) {
            int frame_size = fread(frame_buf.get(), 1, kAdtsHeaderSize, audio_fp.get());
            if (frame_size < kAdtsHeaderSize) {
                LOGI("Read end of aac file");
                break;
            }

            if (!ParseAdtsHeader((uint8_t *)frame_buf.get(), kAdtsHeaderSize, &adts_header)) {
                LOGI("Invalid adts header");
                break;
            }
            int ret = fread(frame_buf.get(), 1, adts_header.aac_frame_length - kAdtsHeaderSize, audio_fp.get());
            if (ret <= 0) {
                LOGE("Read end of file without enough data");
                break;
            }

            SendAACFrame(rtp_packet.get(), frame_buf.get(), ret, true);

            usleep(2000);
        }
    });
    video_thread.join();
    audio_thread.join();
}

void RtspServer::SendH264Frame(RtpPacket *rtp_packet, const char *frame_buf, int frame_size, bool tcp) {
    int start_code_length = 4;
    if (H264ThreeByteStartCode(frame_buf)) {
        start_code_length = 3;
    }

    frame_buf = frame_buf + start_code_length;
    frame_size -= start_code_length;

    uint8_t nalu_type = *(frame_buf);
    if (frame_size <= RTP_MAX_PKT_SIZE) {  // 单nalu模式
        //*   0 1 2 3 4 5 6 7 8 9
        //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        //*  |F|NRI|  Type   | a single NAL unit ... |
        //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        memcpy(rtp_packet->payload, frame_buf, frame_size);
        SendRtpPacket(rtp_packet, frame_size + RTP_HEADER_SIZE, tcp);
        rtp_packet->header.seq++;
    } else {
        int pkg_count = ceil(frame_size * 1.0 / RTP_MAX_PKT_SIZE);
        int position = 1;  // skip nalu_type
        int size = RTP_MAX_PKT_SIZE;
        int remain_frame_size = frame_size;
        rtp_packet->payload[0] = (nalu_type & 0x60) | 0x1C;
        for (int i = 0; i < pkg_count; i++) {
            rtp_packet->payload[1] = (nalu_type & 0x1F);

            if (i == 0) {  // start code
                rtp_packet->payload[1] |= 0x80;
            } else if (i == pkg_count - 1) {  // end code
                rtp_packet->payload[1] |= 0x40;
            }

            size = (remain_frame_size >= RTP_MAX_PKT_SIZE ? RTP_MAX_PKT_SIZE : remain_frame_size);

            memcpy(rtp_packet->payload + 2, frame_buf + position, size);
            SendRtpPacket(rtp_packet, size + 2 + RTP_HEADER_SIZE, tcp);

            position += size;
            remain_frame_size -= size;
            rtp_packet->header.seq++;
        }
        assert(remain_frame_size == 0);
    }
    if ((nalu_type & 0x1F) != 0x07 && (nalu_type & 0x1F) != 0x08) {
        rtp_packet->header.timestamp += kRtmpVideoTimeBase / kFrameRate;
    }
}

void RtspServer::SendAACFrame(RtpPacket *rtp_packet, const char *frame_buf, const int frame_buf_size, bool tcp) {
    // https://blog.csdn.net/yangguoyu8023/article/details/106517251/
    rtp_packet->payload[0] = 0x00;
    rtp_packet->payload[1] = 0x10;
    rtp_packet->payload[2] = (frame_buf_size & 0x1FE0) >> 5;  // 高8位
    rtp_packet->payload[3] = (frame_buf_size & 0x1f) << 3;    // 低5位

    memcpy(rtp_packet->payload + 4, frame_buf, frame_buf_size);

    if (!SendRtpPacket(rtp_packet, frame_buf_size + 4 + RTP_HEADER_SIZE, tcp, 0x02)) {
        LOGE("Send rtp packet failed");
    }

    rtp_packet->header.seq++;

    rtp_packet->header.timestamp = 1025;
}

bool RtspServer::SendRtpPacket(RtpPacket *rtp_packet, int rtp_packet_size, bool tcp, int channel) {
    rtp_packet->header.seq = htons(rtp_packet->header.seq);
    rtp_packet->header.timestamp = htonl(rtp_packet->header.timestamp);
    rtp_packet->header.ssrc = htonl(rtp_packet->header.ssrc);

    std::lock_guard<std::mutex> lock_gard(send_mutex);
    bool ret = true;
    if (!tcp) {
        ret = SendDataByUdp(server_rtp_socket_, client_ip_.c_str(), client_rtp_port_, (const char *)rtp_packet,
                            rtp_packet_size);
    } else {
        char *temp_buf = (char *)malloc(4 + rtp_packet_size);
        temp_buf[0] = 0x24;
        temp_buf[1] = channel;
        temp_buf[2] = (uint8_t)(((rtp_packet_size)&0xFF00) >> 8);
        temp_buf[3] = (uint8_t)((rtp_packet_size)&0xFF);
        memcpy(temp_buf + 4, rtp_packet, rtp_packet_size);

        int send_ret = send(client_rtsp_socket_, temp_buf, 4 + rtp_packet_size, 0);
        if (send_ret < 0) {
            ret = false;
        }
        free(temp_buf);
    }
    rtp_packet->header.seq = ntohs(rtp_packet->header.seq);
    rtp_packet->header.timestamp = ntohl(rtp_packet->header.timestamp);
    rtp_packet->header.ssrc = ntohl(rtp_packet->header.ssrc);
    return ret;
}

//////////////// H264 Help function ////////////////////////
static inline bool H264FourByteStartCode(const char *buf) {
    if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x00 && buf[3] == 0x01) {
        return true;
    }
    return false;
}

static inline bool H264ThreeByteStartCode(const char *buf) {
    if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01) {
        return true;
    }
    return false;
}

static const char *H264FindStartCode(const char *buf, int buf_length) {
    if (buf_length < 3) {
        return nullptr;
    }

    for (int i = 0; i < buf_length - 4; i++) {
        if (H264FourByteStartCode(buf + i) || H264ThreeByteStartCode(buf + i)) {
            return buf + i;
        }
    }

    /// check last three byte
    const char *last_three_byte = buf + buf_length - 3;
    if (H264ThreeByteStartCode(last_three_byte)) {
        return last_three_byte;
    }

    return nullptr;
}

// get h264 frame from .h264 file
static int H264GetFrameFromFile(FILE *fp, char *frame_buf, const int frame_buf_size) {
    int read_size = fread(frame_buf, 1, frame_buf_size, fp);
    if (read_size <= 4) {
        return -1;
    }

    if (!H264ThreeByteStartCode(frame_buf) && !H264FourByteStartCode(frame_buf)) {
        return -1;
    }

    // skip first 3 byte
    const char *next_frame_buf = H264FindStartCode(frame_buf + 3, read_size - 3);
    if (next_frame_buf == nullptr) {
        ///< TODO(tbago) need change
        return -1;
    } else {  // seek to next frame pos
        int seek_pos = next_frame_buf - frame_buf - read_size;
        fseek(fp, seek_pos, SEEK_CUR);
        read_size = next_frame_buf - frame_buf;
    }

    return read_size;
}

//////////////// Help function
static int CreateUdpSocket() {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        LOGE("Create udp socket failed");
        return -1;
    }

    int on = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    return socket_fd;
}

static bool BindSocketAddress(int socket_fd, const char *ip_address, int port) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_address);

    if (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return false;
    }

    return true;
}

static bool SendDataByUdp(int socket, const char *ip_address, const int port, const char *data, const int data_size) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_address);

    int ret = sendto(socket, data, data_size, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        LOGE("Send udp packet failed with errcode : %d", errno);
        return false;
    }
    return true;
}
