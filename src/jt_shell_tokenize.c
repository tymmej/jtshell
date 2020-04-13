#include "jt_shell_tokenize.h"
#include "jt_shell.h"
#include "jt_logger.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "stdio.h"

static bool
jt_shell_tokenize_arg_is_empty(const char *arg)
{

    while (*arg) {
        bool match = false;

        for (const char *c = JT_SHELL_TOKEN_DELIM; '\0' != *c; c++) {
            if (*arg == *c) {
                match = true;
                break;
            }
        }
        if (!match) {
            return false;
        }

        arg++;
    }
    
    return true;
}

void
jt_shell_tokenice_arg_end(char *arg,
                          jt_shell_cmds_t *group,
                          jt_shell_cmd_t *cur_cmd,
                          size_t group_cnt,
                          size_t *args_sz,
                          bool *in, bool *out)
{
    if (!jt_shell_tokenize_arg_is_empty(arg)) {
        if (*in) {
            group[group_cnt - 1].redirect_in = arg;
            *in = false;
        } else if (*out) {
            group[group_cnt - 1].redirect_out = arg;
            *out = false;
        } else {
            cur_cmd->tokens[(*args_sz)++] = arg;
        }
    }
}

jt_shell_cmds_t *
jt_shell_tokenize(char *line)
{
    const char *delims = JT_SHELL_TOKEN_DELIM;

    jt_shell_cmds_t *cmds = calloc(sizeof(jt_shell_cmds_t), 1);
    cmds->cmds = malloc(JT_SHELL_CMDS_COUNT * sizeof(jt_shell_cmd_t *));

    size_t group_cnt = 0;
    size_t args_cnt = 0;
    size_t args_sz;
    char **args = NULL;
    char *arg_start = NULL;

    bool new_group = true;
    bool new_cmd = true;

    bool redirect_in = false;
    bool redirect_out = false;

    jt_shell_cmd_t *cur_cmd = NULL;

    do {
        if (new_group) {
            new_cmd = true;
            group_cnt++;
            cur_cmd = malloc(sizeof(jt_shell_cmd_t));
            cmds->cmds[group_cnt - 1] = cur_cmd;
            cur_cmd->prev = NULL;
            cur_cmd->next = NULL;
        }
        if (new_cmd) {
            if (!new_group) {
                cur_cmd->next = malloc(sizeof(jt_shell_cmd_t));
                ((jt_shell_cmd_t *)cur_cmd->next)->prev = cur_cmd;
                cur_cmd = cur_cmd->next;
                cur_cmd->next = NULL;
            }
            
            args_cnt = JT_SHELL_ARGS_COUNT;
            args_sz = 0;
            args = calloc(args_cnt * sizeof(char *), 1);

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
            if (*c == *line && *(line - 1) != '\0') {
                *line = '\0';
                jt_shell_tokenice_arg_end(arg_start,
                                          cmds,
                                          cur_cmd,
                                          group_cnt,
                                          &args_sz,
                                          &redirect_in, &redirect_out);
                arg_start = line + 1;
                if (args_cnt >= args_sz) {
                    args_cnt += JT_SHELL_ARGS_COUNT;
                    args = realloc(args, args_cnt * sizeof(char *));
                    cur_cmd->tokens = args;
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
            jt_shell_tokenice_arg_end(arg_start,
                                        cmds,
                                        cur_cmd,
                                        group_cnt,
                                        &args_sz,
                                        &redirect_in, &redirect_out);
            arg_start = line + 1;
            new_group = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new group");
        }

        if ('|' == *line) {
            *line = '\0';
            jt_shell_tokenice_arg_end(arg_start,
                                        cmds,
                                        cur_cmd,
                                        group_cnt,
                                        &args_sz,
                                        &redirect_in, &redirect_out);
            arg_start = line + 1;
            new_cmd = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new cmd");
        }

        if ('<' == *line) {
            *line = '\0';
            jt_shell_tokenice_arg_end(arg_start,
                                        cmds,
                                        cur_cmd,
                                        group_cnt,
                                        &args_sz,
                                        &redirect_in, &redirect_out);
            arg_start = line + 1;
            redirect_in = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                        "%s\n",
                        "new redirect_in");
        }

        if ('>' == *line) {
            *line = '\0';
            jt_shell_tokenice_arg_end(arg_start,
                                        cmds,
                                        cur_cmd,
                                        group_cnt,
                                        &args_sz,
                                        &redirect_in, &redirect_out);
            arg_start = line + 1;
            redirect_out = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                        "%s\n",
                        "new redirect_out");
        }

    } while (*(++line));

    jt_shell_tokenice_arg_end(arg_start,
                                cmds,
                                cur_cmd,
                                group_cnt,
                                &args_sz,
                                &redirect_in, &redirect_out);

    cmds->group_cnt = group_cnt;

    jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                        args);

    jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                  "%s %d\n",
                  "Groups found", cmds->group_cnt);

    return cmds;
}
