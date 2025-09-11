#ifndef WATCHER_H
#define WATCHER_H

#include "nut_server.h"

#include <pthread.h>
#include <signal.h>

typedef struct 
{
    nut_ups_t* ups;
    volatile sig_atomic_t* terminate;
} watcher_args_t;

// Starts the signal watcher thread. Returns pthread_create result, pthread_t is set via out parameter.
int signal_watcher_start(watcher_args_t* args, pthread_t* tid);

// Stops the signal watcher thread and waits for it to finish.
void signal_watcher_stop(watcher_args_t* args, pthread_t tid);

#endif // WATCHER_H
