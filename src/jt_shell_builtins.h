#pragma once

typedef int (*jt_shell_builtin_func_t)(char **args);

jt_shell_builtin_func_t jt_shell_builtin_check(char *cmd);
