#include "utility.h"

#include <chrono>
#include <ratio>

int64_t GetCurrentTimeStamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string GetCurrentFormatTime(
    const char *format /* ="%Y-%m-%d %H:%M:%S" */) {
    time_t t = time(nullptr);
    char output[64];
    strftime(output, sizeof(output), format, localtime(&t));

    std::string str_time = output;
    return str_time;
}
