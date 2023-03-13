#include <iostream>
#include "utility/utility.h"
#include "hls_server.h"

/// http server port
static const int kServerPort = 8088;

int main(int argc, const char *argv[]) {
    HlsServer server(kServerPort);
    server.StartServerLoop();
    return 0;
}
