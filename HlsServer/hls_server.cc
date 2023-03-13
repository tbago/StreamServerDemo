#include "hls_server.h"
#include <unistd.h>  //close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utility/log.h"

bool HlsServer::StartServerLoop() {
    int ret = CreateServerSocket();
    if (ret < 0) {
        LOGE("Create server socket failed");
        return false;
    }

    LOGI("Create server socket success. Listen on port %d", server_port_);

    while (true) {
        int client_socket = AcceptClientSocket();
        if (client_socket < 0) {
            LOGE("accept client failed");
            break;
        }
        ReceiveData(client_socket);
    }
    CloseServer();
    return true;
}

int HlsServer::CreateServerSocket() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        LOGE("create server socket failed");
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server_port_);

    if (bind(server_socket_, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGE("Bind server port %d failed", server_port_);
        return -1;
    }

    if (listen(server_socket_, 5) < 0) {
        LOGE("Listen server failed");
        return -1;
    }
    return 0;
}

void HlsServer::CloseServer() {
    close(server_socket_);

}

int HlsServer::AcceptClientSocket() {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    socklen_t length = sizeof(addr);
    int client_socket = accept(server_socket_, (struct sockaddr *)&addr, &length); 
    if (client_socket < 0) {
        return -1;
    }
    const char *ipaddress = inet_ntoa(addr.sin_addr);
    LOGI("Accept connection from %s:%d", ipaddress, addr.sin_port);
    return client_socket;
}

bool HlsServer::ReceiveData(int client_socket) {
    const int kBuffSize = 2000;
    char receive_buf[kBuffSize];
    int32_t buf_size = recv(client_socket, receive_buf, kBuffSize, 0);

    char uri[100];
    //simple check http header and get uri
    const char *separate = "\n";
    char *line = strtok(receive_buf, separate);
    while (line != nullptr) {
        if (strstr(line, "GET") != 0) {
            if (sscanf(line, "GET %s HTTP/1.1\r\n", uri) != 1) {
                LOGE("Parse uri failed, %s", line);
                return false;
            }
        }
    }

    LOGI("uri = %s", uri);

    //use hardcode file path
    std::string file_path = "../data/test" + std::string(uri);
    FILE *fp = fopen(file_path.c_str(), "rb");
    if (fp == nullptr) {
        LOGE("Open file %s failed", file_path.c_str());
        return false;
    }

    int8_t file_buf[kBuffSize * 2];
    int file_buf_size = fread(file_buf, 1, sizeof(file_buf), fp);
    fclose(fp);

    const int32_t kHttpHeaderLength = 2000;
    char http_header[kHttpHeaderLength];
    if (strcmp("/index.m3u8", uri) == 0) {
        sprintf(http_header, "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: * \r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: application/vnd.apple.mpegurl; charset=utf-8\r\n"
            "Keep-Alive: timeout=30, max=100\r\n"
            "Server: hlsServer\r\n"
            "\r\n",
            file_buf_size);
    }
    else {   //for ts file
        sprintf(http_header, "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: * \r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: video/mp2t; charset=utf-8\r\n"
            "Keep-Alive: timeout=30, max=100\r\n"
            "Server: hlsServer\r\n"
            "\r\n",
            file_buf_size);
    }

    send(client_socket, http_header, strlen(http_header), 0);
    send(client_socket, file_buf, file_buf_size, 0);

    sleep(10);

    return 0;
}
