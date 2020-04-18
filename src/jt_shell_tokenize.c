#include "jt_shell_tokenize.h"
#include "jt_shell.h"
#include "jt_logger.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stdio.h"

typedef enum {
    CHAR = 1,
    ESCAPE,
    QUOTE,
    SPACE,
    PIPE,
    SREDNIK, //TODO
    REDIRECT_IN,
    REDIRECT_OUT,
    EOL,
} jt_shell_tokenize_type_t;

typedef struct {
    bool in_quote;
    bool redirect_active;
    bool arg_active;
} jt_shell_tokenize_ctx_t;

static jt_shell_tokenize_ctx_t jt_shell_tokenize_ctx;

static jt_shell_tokenize_type_t
jt_shell_tokenize_get_token(char ch)
{
    jt_shell_tokenize_type_t token = CHAR;
    const char *delims = JT_SHELL_TOKEN_DELIM;
    
    if (ch == '\0') {
        return EOL;
    } else if (ch == ';' && !jt_shell_tokenize_ctx.in_quote) {
        return SREDNIK;
    } else if (ch == '|' && !jt_shell_tokenize_ctx.in_quote) {
        return PIPE;
    } else if (ch == '<' && !jt_shell_tokenize_ctx.in_quote) {
        return REDIRECT_IN;
    } else if (ch == '>' && !jt_shell_tokenize_ctx.in_quote) {
        return REDIRECT_OUT;
    } else if (ch == '"') {
        return QUOTE;
    }

    for (const char *c = delims; '\0' != *c; c++) {
        if (*c == ch && !jt_shell_tokenize_ctx.in_quote) {
            return SPACE;
        }
    }
    return token;
}

void
jt_shell_tokenice_arg_end(char *arg,
                          jt_shell_cmds_t *group,
                          jt_shell_cmd_t *cur_cmd,
                          size_t group_cnt,
                          size_t *args_cnt,
                          bool *in, bool *out)
{
    if (*in) {
        group[group_cnt - 1].redirect_in = arg;
        *in = false;
    } else if (*out) {
        group[group_cnt - 1].redirect_out = arg;
        *out = false;
    } else {
        cur_cmd->tokens[(*args_cnt)++] = arg;
    }
}

jt_shell_cmds_t *
jt_shell_tokenize(char *line)
{

    jt_shell_cmds_t *cmds = calloc(sizeof(jt_shell_cmds_t), 1);
    cmds->cmds = malloc(JT_SHELL_CMDS_COUNT * sizeof(jt_shell_cmd_t *));

    size_t group_cnt = 0;
    size_t args_cnt = 0;
    size_t args_sz = 0;
    char **args = NULL;
    char *arg_start = NULL;

    bool new_group = true;
    bool new_cmd = true;

    bool redirect_in = false;
    bool redirect_out = false;

    jt_shell_cmd_t *cur_cmd = NULL;

    jt_shell_tokenize_type_t token;
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
            
            args_cnt = 0;
            args_sz = JT_SHELL_ARGS_COUNT;
            args = calloc(args_sz * sizeof(char *), 1);

            if (NULL == args) {
                jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                            "%s\n",
                            "Args split cannot alloc");
                return NULL;
            }

            cur_cmd->tokens = args;
            new_cmd = false;
            new_group = false;
        }
        char ch = *line;
        token = jt_shell_tokenize_get_token(ch);
        
        switch (token) {
        case EOL:
            if (jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenice_arg_end(arg_start,
                                            cmds,
                                            cur_cmd,
                                            group_cnt,
                                            &args_cnt,
                                            &redirect_in, &redirect_out);
            }
            memset(&jt_shell_tokenize_ctx, 0, sizeof(jt_shell_tokenize_ctx));
            break;
        case CHAR:
            if (!jt_shell_tokenize_ctx.arg_active) {
                arg_start = line;
            }
            jt_shell_tokenize_ctx.arg_active = true;
            break;
        case QUOTE:
            if (jt_shell_tokenize_ctx.in_quote) {
                *line = '\0';
                jt_shell_tokenize_ctx.in_quote = false;
            } else {
                jt_shell_tokenize_ctx.in_quote = true;
                *line = '\0';
                arg_start = line + 1;
            }
            break;
        case SPACE:
            *line = '\0';
            if (!jt_shell_tokenize_ctx.arg_active) {
                arg_start = line + 1;
                break;
            }
            jt_shell_tokenice_arg_end(arg_start,
                                      cmds,
                                      cur_cmd,
                                      group_cnt,
                                      &args_cnt,
                                      &redirect_in, &redirect_out);
            if (args_cnt >= args_sz) {
                args_sz += JT_SHELL_ARGS_COUNT;
                args = realloc(args, args_sz * sizeof(char *));
                cur_cmd->tokens = args;
                if (NULL == args) {
                    jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                                "%s\n",
                                "Args split cannot realloc");
                    return NULL;
                }
            }
            jt_shell_tokenize_ctx.arg_active = false;
            break;
        case PIPE:
            *line = '\0';
            if (jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenice_arg_end(arg_start,
                                            cmds,
                                            cur_cmd,
                                            group_cnt,
                                            &args_cnt,
                                            &redirect_in, &redirect_out);
            }
            arg_start = line + 1;
            new_cmd = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new cmd");
            break;
        case SREDNIK:
            *line = '\0';
            if (jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenice_arg_end(arg_start,
                                            cmds,
                                            cur_cmd,
                                            group_cnt,
                                            &args_cnt,
                                            &redirect_in, &redirect_out);
            }
            arg_start = line + 1;
            new_group = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new group");
            jt_shell_tokenize_ctx.arg_active = false;
            break;
        case REDIRECT_IN:
            *line = '\0';
            if (jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenice_arg_end(arg_start,
                                            cmds,
                                            cur_cmd,
                                            group_cnt,
                                            &args_cnt,
                                            &redirect_in, &redirect_out);
            }
            arg_start = line + 1;
            redirect_in = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                        "%s\n",
                        "new redirect_in");
            break;
        case REDIRECT_OUT:
            *line = '\0';
            if (jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenice_arg_end(arg_start,
                                            cmds,
                                            cur_cmd,
                                            group_cnt,
                                            &args_cnt,
                                            &redirect_in, &redirect_out);
            }
            arg_start = line + 1;
            redirect_out = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                args);
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                        "%s\n",
                        "new redirect_out");
            break;
        case ESCAPE:
            break;
        }
        line++;
    } while (token != EOL);

    cmds->group_cnt = group_cnt;

    jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                        args);

    jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                  "%s %d\n",
                  "Groups found", cmds->group_cnt);

    return cmds;
}
