#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

const char* cmds[][5] = 
{
    // Low battery notification.
    { "/usr/syno/bin/synonotify", "UPSPowerFail", NULL },
    // Safe shutdown log message.
    { "/usr/syno/bin/synologset1", "sys", "warn", "0x11300011", NULL },
    // Enter safe mode.
    { "/usr/syno/sbin/synopoweroff", "-s", NULL }
};

int main(void)
{
    if (setresuid(0, 0, 0) != 0) {
        perror("setresuid");
        exit(1);
    }

    if (setresgid(0, 0, 0) != 0) {
        perror("setresgid");
        exit(1);
    }

    size_t num_cmds = sizeof(cmds) / sizeof(cmds[0]);

    for (size_t i = 0; i < num_cmds; ++i)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            return 1;
        }

        if (pid == 0)
        {
            execv(cmds[i][0], (char* const*)cmds[i]);
            perror("execv");
            _exit(1);
        }
        
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid");
            return 1;
        }
    }

    return 0;
}
