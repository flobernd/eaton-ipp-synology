#include "watcher.h"
#include "nut_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <limits.h>
#include <libgen.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <pthread.h>

static const nut_ups_variable_t status_ol = { "ups.status", NUT_VAR_TYPE_STRING, "OL" };
static const nut_ups_variable_t status_oblb = { "ups.status", NUT_VAR_TYPE_STRING, "OB LB" };

static void* signal_watcher_thread_fn(void* arg)
{
    watcher_args_t* args = (watcher_args_t*)arg;

    nut_ups_t* ups = args->ups;
    volatile sig_atomic_t* terminate = args->terminate;

    // Get directory of the current executable.
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len < 0)
    {
        perror("readlink");
        return NULL;
    }
    buf[len] = '\0';

    char* dir = dirname(buf);

    // Delete existing 'lb_timestamp' file on startup to ensure we start in "OL" state.
    char lb_path[PATH_MAX];
    snprintf(lb_path, sizeof(lb_path), "%s/lb_timestamp", dir);
    if (access(lb_path, F_OK) == 0)
    {
        if (remove(lb_path) != 0)
        {
            perror("remove(lb_timestamp)");
        }
    }

    int inotify_fd = inotify_init();
    if (inotify_fd < 0)
    {
        perror("inotify_init");
        return NULL;
    }

    int wd = inotify_add_watch(inotify_fd, dir, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO);
    if (wd < 0)
    {
        perror("inotify_add_watch");
        close(inotify_fd);
        return NULL;
    }

    char event_buf[4096]
    __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t num_read;

    while (!*terminate)
    {
        num_read = read(inotify_fd, event_buf, sizeof(event_buf));
        if (num_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("read (inotify)");
            break;
        }

        if (*terminate)
        {
            break;
        }

        struct inotify_event* last_event = NULL;

        for (char* ptr = event_buf; ptr < event_buf + num_read; )
        {
            struct inotify_event* event = (struct inotify_event*)ptr;
            if (event->len > 0 && strcmp(event->name, "lb_timestamp") == 0)
            {
                last_event = event;
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }

        if (last_event)
        {
            if ((last_event->mask & IN_CREATE) || (last_event->mask & IN_MODIFY)  || (last_event->mask & IN_MOVED_TO))
            {
                kv_store_set(ups->ups_variables, "ups.status", (void*)&status_oblb);
            }

            if (last_event->mask & IN_DELETE)
            { 
                kv_store_set(ups->ups_variables, "ups.status", (void*)&status_ol);
            }
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);

    return NULL;
}

int signal_watcher_start(watcher_args_t* args, pthread_t* tid)
{
    return pthread_create(tid, NULL, signal_watcher_thread_fn, args);
}

void signal_watcher_stop(watcher_args_t* args, pthread_t tid)
{
    if (!args || !args->terminate)
    {
        return;
    }

    *(args->terminate) = 1;
    pthread_kill(tid, SIGUSR1);
    pthread_join(tid, NULL);
}
