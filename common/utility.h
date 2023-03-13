#ifndef COMMON_UTILITY_H_
#define COMMON_UTILITY_H_

#include <cstdint>
#include <string>

/*
 * Get time stamp since 1970-01-01 (ms)
 */
int64_t GetCurrentTimeStamp();

/*
 * Get current format time
 */
std::string GetCurrentFormatTime(const char *format = "%Y-%m-%d %H:%M:%S");

#endif  // COMMON_UTILITY_H_
