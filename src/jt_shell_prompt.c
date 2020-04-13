#include "jt_shell_prompt.h"
#include "jt_shell.h"
#include "jt_logger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

typedef enum {
    USER = 1,
    HOSTNAME,
} jt_shell_get_t;

static const char *jt_shell_prompt_text = "jtshell>";

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
                assert(strlen(pwd.pw_name) + 1 < sizeof(buffer));
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
                line_buf = malloc(line_sz);
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

int
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
