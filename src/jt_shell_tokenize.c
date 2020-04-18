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
    size_t group_cnt;
    size_t args_cnt;
    size_t args_sz;
    char **args;
    char *arg_start;

    bool new_group;
    bool new_cmd;

    bool redirect_in;
    bool redirect_out;
    
    jt_shell_cmds_t *cmds;
    
    jt_shell_cmd_t *cur_cmd;

    jt_shell_tokenize_type_t token;
    
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
jt_shell_tokenice_arg_end(jt_shell_tokenize_ctx_t *ctx)
{
    
    if (!jt_shell_tokenize_ctx.arg_active) {
        return;
    }
    
    if (ctx->redirect_in) {
        ctx->cmds[ctx->group_cnt - 1].redirect_in = ctx->arg_start;
        ctx->redirect_in = false;
    } else if (ctx->redirect_out) {
        ctx->cmds[ctx->group_cnt - 1].redirect_out = ctx->arg_start;
        ctx->redirect_out = false;
    } else {
        ctx->cur_cmd->tokens[(ctx->args_cnt)++] = ctx->arg_start;
    }
    
    if (ctx->args_cnt >= ctx->args_sz) {
        ctx->args_sz += JT_SHELL_ARGS_COUNT;
        ctx->args = realloc(ctx->args, ctx->args_sz * sizeof(char *));
        ctx->args[ctx->args_cnt] = NULL;
        ctx->cur_cmd->tokens = ctx->args;
        if (NULL == ctx->args) {
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "Args split cannot realloc");
            //return NULL;
            exit(1);
        }
    }
}

jt_shell_cmds_t *
jt_shell_tokenize(char *line)
{

    memset(&jt_shell_tokenize_ctx, 0, sizeof(jt_shell_tokenize_ctx));
    jt_shell_tokenize_ctx.cmds = calloc(sizeof(jt_shell_cmds_t), 1);
    jt_shell_tokenize_ctx.cmds->cmds = malloc(JT_SHELL_CMDS_COUNT * sizeof(jt_shell_cmd_t *));
    
    jt_shell_tokenize_ctx.new_group = true;
    
    do {
        if (jt_shell_tokenize_ctx.new_group) {
            jt_shell_tokenize_ctx.new_cmd = true;
            jt_shell_tokenize_ctx.group_cnt++;
            jt_shell_tokenize_ctx.cur_cmd = malloc(sizeof(jt_shell_cmd_t));
            jt_shell_tokenize_ctx.cmds->cmds[jt_shell_tokenize_ctx.group_cnt - 1] = jt_shell_tokenize_ctx.cur_cmd;
            jt_shell_tokenize_ctx.cur_cmd->prev = NULL;
            jt_shell_tokenize_ctx.cur_cmd->next = NULL;
        }
        if (jt_shell_tokenize_ctx.new_cmd) {
            if (!jt_shell_tokenize_ctx.new_group) {
                jt_shell_tokenize_ctx.cur_cmd->next = malloc(sizeof(jt_shell_cmd_t));
                ((jt_shell_cmd_t *)jt_shell_tokenize_ctx.cur_cmd->next)->prev = jt_shell_tokenize_ctx.cur_cmd;
                jt_shell_tokenize_ctx.cur_cmd = jt_shell_tokenize_ctx.cur_cmd->next;
                jt_shell_tokenize_ctx.cur_cmd->next = NULL;
            }
            
            jt_shell_tokenize_ctx.args_cnt = 0;
            jt_shell_tokenize_ctx.args_sz = JT_SHELL_ARGS_COUNT;
            jt_shell_tokenize_ctx.args = calloc(jt_shell_tokenize_ctx.args_sz * sizeof(char *), 1);

            if (NULL == jt_shell_tokenize_ctx.args) {
                jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                            "%s\n",
                            "Args split cannot alloc");
                return NULL;
            }

            jt_shell_tokenize_ctx.cur_cmd->tokens = jt_shell_tokenize_ctx.args;
            jt_shell_tokenize_ctx.new_cmd = false;
            jt_shell_tokenize_ctx.new_group = false;
        }
        
        jt_shell_tokenize_ctx.token = jt_shell_tokenize_get_token(*line);
        
        switch (jt_shell_tokenize_ctx.token) {
        case EOL:
            jt_shell_tokenice_arg_end(&jt_shell_tokenize_ctx);
            break;
        case CHAR:
            if (!jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenize_ctx.arg_start = line;
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
                jt_shell_tokenize_ctx.arg_start = line + 1;
            }
            break;
        case SPACE:
            *line = '\0';
            if (!jt_shell_tokenize_ctx.arg_active) {
                jt_shell_tokenize_ctx.arg_start = line + 1;
                break;
            }
            jt_shell_tokenice_arg_end(&jt_shell_tokenize_ctx);
            jt_shell_tokenize_ctx.arg_active = false;
            break;
        case PIPE:
            *line = '\0';
            jt_shell_tokenice_arg_end(&jt_shell_tokenize_ctx);
            jt_shell_tokenize_ctx.arg_start = line + 1;
            jt_shell_tokenize_ctx.new_cmd = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                jt_shell_tokenize_ctx.args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new cmd");
            break;
        case SREDNIK:
            *line = '\0';
            jt_shell_tokenice_arg_end(&jt_shell_tokenize_ctx);
            jt_shell_tokenize_ctx.arg_start = line + 1;
            jt_shell_tokenize_ctx.new_group = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                jt_shell_tokenize_ctx.args);
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "new group");
            jt_shell_tokenize_ctx.arg_active = false;
            break;
        case REDIRECT_IN:
            *line = '\0';
            jt_shell_tokenice_arg_end(&jt_shell_tokenize_ctx);
            jt_shell_tokenize_ctx.arg_start = line + 1;
            jt_shell_tokenize_ctx.redirect_in = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                jt_shell_tokenize_ctx.args);
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                        "%s\n",
                        "new redirect_in");
            break;
        case REDIRECT_OUT:
            *line = '\0';
            jt_shell_tokenice_arg_end(&jt_shell_tokenize_ctx);
            jt_shell_tokenize_ctx.arg_start = line + 1;
            jt_shell_tokenize_ctx.redirect_out = true;
            jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                                jt_shell_tokenize_ctx.args);
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                        "%s\n",
                        "new redirect_out");
            break;
        case ESCAPE:
            break;
        }
        line++;
    } while (jt_shell_tokenize_ctx.token != EOL);

    jt_shell_tokenize_ctx.cmds->group_cnt = jt_shell_tokenize_ctx.group_cnt;

    jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                        jt_shell_tokenize_ctx.args);

    jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                  "%s %d\n",
                  "Groups found", jt_shell_tokenize_ctx.cmds->group_cnt);

    return jt_shell_tokenize_ctx.cmds;
}
