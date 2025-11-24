#include "log.h"
#include <stdarg.h>
#include <time.h>

void log_write(const char *level,
               const char *file,
               const char *func,
               int line,
               const char *fmt, ...)
{
    va_list ap;
    char msg[1024];

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // 간단히 stderr로만 출력 (필요하면 파일로 변경 가능)
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    fprintf(stderr,
            "[%02d:%02d:%02d] [%s] %s:%d (%s): %s\n",
            lt->tm_hour, lt->tm_min, lt->tm_sec,
            level, file, line, func, msg);
}