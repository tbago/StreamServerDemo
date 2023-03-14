#include "utility.h"
#include "rtsp_server.h"

int main(int argc, const char *argv[]) {
    RtspServer server(8082);
    server.StartServerLoop();
    return 0;
}
