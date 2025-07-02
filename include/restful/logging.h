#ifndef RESTFUL_LOGGING_H
#define RESTFUL_LOGGING_H

#include <sys/syslog.h>

#ifdef NDEBUG
#define RESTFUL_DEBUG(fmt, ...) ((void)0)
#else
#define RESTFUL_DEBUG(fmt, ...)                                                \
    syslog(LOG_DEBUG, "(%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

#define RESTFUL_INFO(fmt, ...)                                                 \
    syslog(LOG_INFO, "(%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define RESTFUL_WARN(fmt, ...)                                                 \
    syslog(LOG_WARNING, "(%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define RESTFUL_ERR(fmt, ...)                                                  \
    syslog(LOG_ERR, "(%s:%d) " fmt, __func__, __LINE__, ##__VA_ARGS__)

#endif // RESTFUL_LOGGING_H
