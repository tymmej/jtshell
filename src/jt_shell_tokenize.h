#pragma once

#include <unistd.h>

typedef struct {
    void *prev;
    void *next;
    char **tokens;
} jt_shell_cmd_t;

//TODO redirect only works for all groups
typedef struct {
    char *stdin;
    char *stdout;
    size_t group_cnt;
    jt_shell_cmd_t **cmds;
} jt_shell_cmds_t;

jt_shell_cmds_t *jt_shell_tokenize(char *line);
