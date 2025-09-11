// Eaton IPP to NUT proxy.

#include "kv_store.h"
#include "nut_server.h"
#include "nut_proto.h"
#include "watcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>

#define NUT_DEFAULT_PORT 3493

static const nut_ups_variable_t ups_vars[] = 
{
    { "battery.charge"        , NUT_VAR_TYPE_NUMBER, "100.00" },
    { "battery.runtime"       , NUT_VAR_TYPE_NUMBER, "600.00" },
    { "battery.runtimelow"    , NUT_VAR_TYPE_NUMBER,  "60.00" },
    { "battery.voltage"       , NUT_VAR_TYPE_NUMBER,  "52.00" },
    { "device.mfr"            , NUT_VAR_TYPE_STRING, "EATON"  },
    { "device.model"          , NUT_VAR_TYPE_STRING, "Eaton Intelligent Power Protector Relay" },
    { "device.type"           , NUT_VAR_TYPE_STRING, "ups"    },
    { "input.voltage"         , NUT_VAR_TYPE_NUMBER, "230.00" },
    { "input.voltage.nominal" , NUT_VAR_TYPE_NUMBER, "230.00" },
    { "output.voltage"        , NUT_VAR_TYPE_NUMBER, "230.00" },
    { "output.voltage.nominal", NUT_VAR_TYPE_NUMBER, "230.00" },
    { "ups.load"              , NUT_VAR_TYPE_NUMBER, "50.00"  },
    { "ups.mfr"               , NUT_VAR_TYPE_STRING, "EATON"  },
    { "ups.model"             , NUT_VAR_TYPE_STRING, "Eaton Intelligent Power Protector Relay" },
    { "ups.status"            , NUT_VAR_TYPE_STRING, "OL"     },
    { "ups.type"              , NUT_VAR_TYPE_STRING, "normal" }
};

int main()
{
    // SIGPIPE commonly happens when writing to a client socket after the client already 
    // disconnected. We gracefully handle this case by checking the return value.
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(NUT_DEFAULT_PORT);

    // Synology expects a single UPS named "ups".
    nut_ups_t* ups = nut_ups_create(
        "ups",
        "Eaton Intelligent Power Protector Relay",
        ups_vars,
        sizeof(ups_vars) / sizeof(ups_vars[0])
    );

    if (!ups)
    {
        fprintf(stderr, "Failed to create UPS device.\n");
        return 1;
    }

    const nut_ups_t* const ups_devices[] = { ups };
    size_t ups_devices_count = sizeof(ups_devices) / sizeof(ups_devices[0]);

    // Synology expects an admin user named "monuser" with password "secret".
    nut_server_t* server = nut_server_create(&addr, ups_devices, ups_devices_count, "monuser", "secret");
    if (!server)
    {
        fprintf(stderr, "Failed to create NUT server.\n");
        return 1;
    }

    volatile sig_atomic_t terminate = 0;
    watcher_args_t watcher_args = { ups, &terminate };

    pthread_t signal_watcher_thread;
    if (signal_watcher_start(&watcher_args, &signal_watcher_thread) != 0)
    {
        perror("pthread_create");
        nut_server_destroy(server);
        return 1;
    }

    // Start the server (blocking call).
    int result = nut_server_start(server);
    if (result < 0)
    {
        fprintf(stderr, "Failed to start NUT server.\n");
    }

    // Stop the watcher thread and wait for it to finish.
    signal_watcher_stop(&watcher_args, signal_watcher_thread);

    nut_server_destroy(server);

    return result;
}
