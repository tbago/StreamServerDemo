#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

#include "utility.h"

#define LOGI(format, ...)  fprintf(stderr,"[INFO]%s [%s:%d %s()] " format "\n", GetCurrentFormatTime().c_str(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[ERROR]%s [%s:%d %s()] " format "\n",GetCurrentFormatTime().c_str(),__FILE__,__LINE__,__func__ ,##__VA_ARGS__)

#endif //  COMMON_LOG_H_
