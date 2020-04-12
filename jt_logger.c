#include "jt_logger.h"

#include <stdarg.h>
#include <stdio.h>

static jt_logger_level_t jt_logger_level = JT_LOGGER_LEVEL_ERROR;

void
jt_logger_init(jt_logger_level_t level)
{

    jt_logger_level = level;
}

void
jt_logger_log(jt_logger_level_t level, const char *format, ...)
{

    if (level >= jt_logger_level) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
}


void
jt_logger_log_array(jt_logger_level_t level, char **args)
{

    if (level >= jt_logger_level) {
        while (NULL != *args) {
            fprintf(stderr, "%s\n", *args);
            args++;
        }
    }
}