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

    size_t cmd_count = 0;
    size_t args_cnt;
    size_t args_sz;
    char **args;
    char *arg_start;

    bool new_cmd = true;

    do {
        if (new_cmd) {
            cmds->cmds[cmd_count] = malloc(sizeof(jt_shell_cmd_t));

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

            cmds->cmds[cmd_count]->tokens = args;
            new_cmd = false;
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
            new_cmd = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            cmd_count++;
        }

    } while (*(++line));

    cmd_count++;
    args[args_sz++] = arg_start;

    cmds->cmd_cnt = cmd_count;

    jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                        args);

    jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                "%s %d\n",
                "Command found", cmds->cmd_cnt);

    return cmds;
}