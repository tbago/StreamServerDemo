#include "http_flv_server.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"

const char *flvFilePath = "./data/test.flv";

bool HttpFlvServer::StartServerLoop() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        LOGE("Create server socket failed");
        return false;
    }

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

    LOGI("Create server success. Listen on port %d", server_port_);

    constexpr char http_header[] = "HTTP/1.1 200 OK\r\n" \
		"Access-Control-Allow-Origin: * \r\n" \
		"Content-Type: video/x-flv\r\n" \
		"Content-Length: -1\r\n" \
		"Connection: Keep-Alive\r\n" \
		"Expires: -1\r\n" \
		"Pragma: no-cache\r\n" \
		"\r\n";
    const int http_header_size = strlen(http_header);

    const int kReceiveBufSize = 2000;
    char receive_buf[kReceiveBufSize];
    const int kSendBufSize = 2000;
    char send_buf[kSendBufSize];
    FILE *fp = fopen(flvFilePath, "rb");
    if (fp == nullptr) {
        LOGE("Cannot open flv file");
        return false;
    }
    while (true) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        socklen_t length = sizeof(addr); 
        int client_socket = accept(server_socket_, (struct sockaddr *)&addr, &length);
        if (client_socket < 0) {
            LOGE("Invalid client socket");
            break;
        }
        fseek(fp, 0, SEEK_SET);
        const char *ipaddress = inet_ntoa(addr.sin_addr);
        LOGI("Accept connection from %s:%d", ipaddress, addr.sin_port);

        int receive_size = recv(client_socket, receive_buf, kReceiveBufSize, 0);
        LOGI("Receive http flv request : %s", receive_buf);
        // send http header
        send(client_socket, http_header, http_header_size, 0);
        // loop send flv data
        while (true) {
            usleep(1000);
            int buf_size = fread(send_buf, 1, kSendBufSize, fp);
            if (buf_size <= 0) {
                LOGI("Read end of file");
                break;
            }
            int ret = send(client_socket, send_buf, buf_size, 0);
            if (ret <= 0) {
                LOGE("Send flv file data failed");
                break;
            }
            // LOGI("Send %d data to client", ret);
        }

        close(client_socket);
    }

    fclose(fp);
    return true;
}

void HttpFlvServer::CloseServer() {
    close(server_socket_);
}

