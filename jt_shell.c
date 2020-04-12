#include "jt_shell.h"
#include "jt_shell_builtins.h"
#include "jt_logger.h"

#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/wait.h>

static const char *jt_shell_prompt_text = "jtshell>";

typedef enum {
    USER = 1,
    HOSTNAME,
} jt_shell_get_t;

static char *
jt_shell_get_info(jt_shell_get_t type)
{
    char *line_buf = NULL;

    switch (type) {
        case USER:
            {
            char buffer[1024];
            struct passwd pwd;
            struct passwd *result;
            int res = getpwuid_r(getuid(),
                                 &pwd,
                                 buffer,
                                 sizeof(buffer),
                                 &result);
            if (0 == res || NULL != result) {
                line_buf = malloc(strlen(pwd.pw_name) + 1);
                if (NULL == line_buf) {
                    jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                                  "%s\n",
                                  "Cannot username allocate");
                }
                strcpy(line_buf, pwd.pw_name);
            }
            }
            break;
        case HOSTNAME:
            {
            size_t line_sz = JT_SHELL_LINE_SIZE;
            line_buf = malloc(line_sz);
            int res;
            do {
                if (NULL == line_buf) {
                    jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                                  "%s\n",
                                  "Cannot hostname allocate");
                    return NULL;
                }
                res = gethostname(line_buf, line_sz);
                if (res != ENAMETOOLONG) {
                    break;
                }
                free(line_buf);
                line_sz += JT_SHELL_LINE_SIZE;
                malloc(line_sz);
            } while (true);
            }
            break;
        default:
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                          "%s\n",
                          "Unknown type");
            return NULL;
    }

    jt_logger_log(JT_LOGGER_LEVEL_DEBUG,
                  "%s: %s\n",
                  "prompt", line_buf);
    return line_buf;
}

static int
jt_shell_prompt_print(void)
{
    char *hostname_buf;
    char *username_buf;

    hostname_buf = jt_shell_get_info(HOSTNAME);

    if (NULL == hostname_buf) {
        free(hostname_buf);
        return -1;
    }
    username_buf = jt_shell_get_info(USER);

    if (NULL == username_buf) {
        free(username_buf);
        return -1;
    }
    
    printf("[%s@%s] %s", username_buf, hostname_buf, jt_shell_prompt_text);

    free(hostname_buf);
    free(username_buf);
    
    return 0;
}


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

static char **
jt_shell_args_split(char *line)
{
    const char *delims = JT_SHELL_TOKEN_DELIM;
    size_t args_cnt = JT_SHELL_ARGS_COUNT;
    size_t args_sz = 0;
    char **args = malloc(args_cnt * sizeof(char *));
    char *arg_start = line;

    if (NULL == args) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                    "%s\n",
                    "Args split cannot alloc");
        return NULL;
    }

    while (*line) {
        //for loop is strtok implementation
        for (const char *c = delims; '\0' != *c; c++) {
            if (*c == *line) {
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
            }
        }

        line++;
    }
    args[args_sz++] = arg_start;

    jt_logger_log_array(JT_LOGGER_LEVEL_DEBUG,
                        args);

    return args;
}

static int
jt_shell_exec(char *line)
{
    int status;

    char **args = jt_shell_args_split(line);

    if (NULL == args) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "Error splitting line");
        return -2;
    }

    jt_shell_builtin_func_t func = jt_shell_builtin_check(args[0]);
    if (func) {
        return func(args);
    }

    pid_t pid = fork();

    if (0 == pid) {
        if (execvp(args[0], args) == -1) {
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                          "%s: %s\n",
                          "Error executing cmd", args[0]);
            free(args);
            return -1;
        }
        return 0;
    } else if (0 > pid) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "Error executing cmd, could not fork");
        free(args);
        return -2;    
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        free(args);
        return 0;
    }
}

int
jt_shell(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    jt_logger_init(JT_LOGGER_LEVEL_DEBUG);
    jt_logger_init(JT_LOGGER_LEVEL_WARN);

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
