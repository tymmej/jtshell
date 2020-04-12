#pragma once

#define JT_SHELL_LINE_SIZE (16)
#define JT_SHELL_ARGS_COUNT (4)
#define JT_SHELL_CMDS_COUNT (4)
#define JT_SHELL_TOKEN_DELIM " \t\r\n\a"

#define UNUSED(x) ((void)(x))


int jt_shell(int argc, char *argv[]);
