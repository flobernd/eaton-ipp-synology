#ifndef NUT_CLIENT_CONTEXT_H
#define NUT_CLIENT_CONTEXT_H

#include "nut_server.h"

#include <netinet/in.h>

typedef struct nut_client_context
{
    nut_server_t* server;
    int client_fd;
    struct sockaddr_in client_addr;
    const char* username;
    const char* password;
} nut_client_context_t;

#endif // NUT_CLIENT_CONTEXT_H
