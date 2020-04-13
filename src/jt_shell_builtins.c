#include "jt_shell_builtins.h"
#include "jt_shell.h"
#include "jt_logger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char *name;
    jt_shell_builtin_func_t func;
} jt_shell_builtin_cmd_t;


static int jt_shell_builtin_cd(char **args);
static int jt_shell_builtin_exit(char **args);

jt_shell_builtin_cmd_t jt_shell_builtins[] = {
    {"cd", jt_shell_builtin_cd},
    {"exit", jt_shell_builtin_exit},
    {NULL, NULL},
};


jt_shell_builtin_func_t
jt_shell_builtin_check(char *cmd)
{
    jt_shell_builtin_cmd_t *candidate = jt_shell_builtins;

    while (candidate->name) {
        if (strcmp(candidate->name, cmd) == 0) {
            return candidate->func;
        }
        candidate++;
    }

    return NULL;
}

static int
jt_shell_builtin_cd(char **args)
{

    if (NULL == args[1]) {
        jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                        "%s\n",
                        "Wrong arg for cd");
    } else {
        if (chdir(args[1]) != 0) {
            jt_logger_log(JT_LOGGER_LEVEL_ERROR,
                            "%s\n",
                            "chdir error");
            return -1;
        }
    }
    
    return 0;
}

static int
jt_shell_builtin_exit(char **args)
{
    UNUSED(args);
    exit(0);
    return 0;
}
