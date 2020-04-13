#include "jt_shell.h"
#include "jt_shell_builtins.h"
#include "jt_shell_prompt.h"
#include "jt_shell_tokenize.h"
#include "jt_logger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

char *
jt_shell_line_get(void)
{
    size_t line_sz = JT_SHELL_LINE_SIZE;
    size_t line_len = 0;

    char *line = malloc(line_sz);

    if (NULL == line) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                    "%s\n",
                    "Args split cannot alloc");
        return NULL;
    }

    int c;

    for (;;) {
        c = getchar();
        if (c == EOF) {
            jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                          "%s\n",
                          "Line get EOF");
            return NULL;
        } else if (c =='\n') {
            line[line_len] = '\0';
            return line;
        } else if (0 <= c && 0xff >= c) {
            line[line_len++] = (char)c;

            if (line_len >= line_sz) {
                line_sz += JT_SHELL_LINE_SIZE;
                line = realloc(line, line_sz);
                if (NULL == line) {
                    jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                                "%s\n",
                                "Line get cannot realloc");
                    return NULL;
                }
            }
        } else {
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                          "%s: %d\n",
                          "Line get weird char", c);
            assert(false);
        }
    }
}

static int
jt_shell_exec_single(char **cmd)
{
    int status;
    jt_shell_builtin_func_t func = jt_shell_builtin_check(cmd[0]);

    if (func) {
        return func(cmd);
    }

    pid_t pid = fork();

    if (0 == pid) {
        if (execvp(cmd[0], cmd) == -1) {
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                          "%s: %s\n",
                          "Error executing cmd", cmd[0]);
            exit(EXIT_FAILURE);
        }
        exit(0);
    } else if (0 > pid) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "Error executing cmd, could not fork");
        return -2;    
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        return 0;
    }
}

static int
jt_shell_exec(char *line)
{
    jt_shell_cmds_t *cmds = jt_shell_tokenize(line);

    if (NULL == cmds) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "Error splitting line");
        return -2;
    }
    
    int res;
    size_t i = 0;
    do {
        jt_shell_cmd_t *cmd = cmds->cmds[i];
        res = jt_shell_exec_single(cmd->tokens);
        i++;
    } while (i < cmds->cmd_cnt && 0 == res);

    return res;
}

int
jt_shell(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    jt_logger_init(JT_LOGGER_LEVEL_DEBUG);
    //jt_logger_init(JT_LOGGER_LEVEL_WARN);

    for (;;) {
        int res = jt_shell_prompt_print();
        if (res) {
            exit(EXIT_FAILURE);
        }
        char *line = jt_shell_line_get();
        if (NULL == line) {
            exit(EXIT_FAILURE);
        }
        jt_logger_log(JT_LOGGER_LEVEL_DEBUG, "%s: %s\n", "Line get line", line);
        res = jt_shell_exec(line);
        free(line);
        if (res) {
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
