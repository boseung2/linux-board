#ifndef LOG_H
#define LOG_H

#include <stdio.h>

void log_write(const char *level, const char *file, const char *func, int line,
               const char *fmt, ...);

#define LOG_INFO(fmt, ...)                                                     \
  log_write("INFO", __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                     \
  log_write("WARN", __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                    \
  log_write("ERROR", __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)                                                    \
  log_write("DEBUG", __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#endif