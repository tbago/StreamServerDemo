#include "rtsp_server.h"
#include "utility.h"

int main(int argc, const char *argv[]) {
    RtspServer server(8082);
    server.StartServerLoop();
    return 0;
}
