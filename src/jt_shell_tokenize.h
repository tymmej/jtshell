#pragma once

#include <unistd.h>

typedef struct {
    char **tokens;
} jt_shell_cmd_t;

typedef struct {
    char *stdin;
    char *stdout;
    size_t cmd_cnt;
    jt_shell_cmd_t **cmds;
} jt_shell_cmds_t;

jt_shell_cmds_t *jt_shell_tokenize(char *line);
