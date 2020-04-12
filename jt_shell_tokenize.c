#include "jt_shell_tokenize.h"
#include "jt_shell.h"
#include "jt_logger.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

jt_shell_cmds_t *
jt_shell_tokenize(char *line)
{
    const char *delims = JT_SHELL_TOKEN_DELIM;

    jt_shell_cmds_t *cmds = malloc(sizeof(jt_shell_cmds_t));
    cmds->cmds = malloc(JT_SHELL_CMDS_COUNT * sizeof(jt_shell_cmd_t *));

    size_t group_cnt = 0;
    size_t args_cnt;
    size_t args_sz;
    char **args;
    char *arg_start;

    bool new_group = true;
    bool new_cmd = true;

    jt_shell_cmd_t *cur_cmd;

    do {
        if (new_group) {
            // cmds->cmds[group_count] = malloc(sizeof(jt_shell_cmd_t));
            // cur_cmd = cmds->cmds[group_count];
            // cur_cmd->prev = NULL;
            // cur_cmd->next = NULL;
            new_cmd = true;
            group_cnt++;
        }
        if (new_cmd) {
            if (new_group) {
                cur_cmd = malloc(sizeof(jt_shell_cmd_t));
                cmds->cmds[group_cnt - 1] = cur_cmd;
                cur_cmd->prev = NULL;
                cur_cmd->next = NULL;
            } else {
                cur_cmd->next = malloc(sizeof(jt_shell_cmd_t));
                ((jt_shell_cmd_t *)cur_cmd->next)->prev = cur_cmd;
                cur_cmd = cur_cmd->next;
                cur_cmd->next = NULL;
            }
            
            args_cnt = JT_SHELL_ARGS_COUNT;
            args_sz = 0;
            args = malloc(args_cnt * sizeof(char *));

            if (NULL == args) {
                jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                            "%s\n",
                            "Args split cannot alloc");
                return NULL;
            }
            
            arg_start = line;

            cur_cmd->tokens = args;
            new_cmd = false;
            new_group = false;
        }

        //for loop is strtok implementation
        for (const char *c = delims; '\0' != *c; c++) {
            //TODO commands starting with delim
            if (*c == *line && *(line - 1) != '\0') {
                args[args_sz++] = arg_start;
                arg_start = line + 1;
                *line = '\0';
                if (args_cnt >= args_sz) {
                    args_cnt += JT_SHELL_ARGS_COUNT;
                    args = realloc(args, args_cnt * sizeof(char *));
                    if (NULL == args) {
                        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                                    "%s\n",
                                    "Args split cannot realloc");
                        return NULL;
                    }
                }
                break;
            } else if (*c == *line) {
                *line = '\0';
                arg_start++;
                break;
            }
        }

        if (';' == *line) {
            *line = '\0';
            args[args_sz++] = arg_start;
            new_group = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new group");
        }

        if ('|' == *line) {
            *line = '\0';
            args[args_sz++] = arg_start;
            new_cmd = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new group");
        }

    } while (*(++line));

    args[args_sz++] = arg_start;

    cmds->group_cnt = group_cnt;

    jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                        args);

    jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                  "%s %d\n",
                  "Commands found", cmds->group_cnt);

    return cmds;
}