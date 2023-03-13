#include "log.h"
#include "http_flv_server.h"

static const int kHttpFlvServerPort = 8089;

int main(int argc, const char *argv[]) {
    LOGI("Start http flv server at : %d", kHttpFlvServerPort);
    LOGI("Access http flv server at http://127.0.0.1:%d/test.flv", kHttpFlvServerPort);
    HttpFlvServer server(kHttpFlvServerPort);
    server.StartServerLoop();
    return 0;
}
