#pragma once

typedef enum {
    JT_LOGGER_LEVEL_DEBUG = 0,
    JT_LOGGER_LEVEL_INFO,
    JT_LOGGER_LEVEL_WARN,
    JT_LOGGER_LEVEL_ERROR,
    JT_LOGGER_LEVEL_CRITICAL,
} jt_logger_level_t;

void jt_logger_init(jt_logger_level_t level);
void jt_logger_log(jt_logger_level_t level, const char *format, ...);
void jt_logger_log_array(jt_logger_level_t level, char **args);
