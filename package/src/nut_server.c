#include "nut_client_context.h"
#include "nut_server.h"
#include "nut_proto.h"
#include "nut_handler.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>

#define BUF_SIZE_READ 256

nut_ups_t* nut_ups_create(const char* name, const char* description, const nut_ups_variable_t* ups_variables, size_t ups_variables_count)
{
    if (!name || !description || !ups_variables || ups_variables_count == 0)
    {
        return NULL;
    }

    nut_ups_t* ups = malloc(sizeof(nut_ups_t));
    if (!ups)
    {
        return NULL;
    }

    ups->name = strdup(name);
    ups->description = strdup(description);

    // Initialize empty kv_store and populate it using kv_store_set
    ups->ups_variables = kv_store_init(NULL, 0);
    if (!ups->ups_variables)
    {
        nut_ups_destroy(ups);
        return NULL;
    }

    for (size_t i = 0; i < ups_variables_count; ++i)
    {
        const nut_ups_variable_t* var = &ups_variables[i];
        kv_store_set(ups->ups_variables, var->name, var);
    }

    if (!ups->name || !ups->description)
    {
        nut_ups_destroy(ups);
        return NULL;
    }

    return ups;
}

void nut_ups_destroy(nut_ups_t* ups)
{
    if (!ups)
    {
        return;
    }

    if (ups->name)
    {
        free(ups->name);
    }

    if (ups->description)
    {
        free(ups->description);
    }

    if (ups->ups_variables)
    {
        kv_store_destroy(ups->ups_variables);
    }

    free(ups);
}

static void* nut_server_connection(void* arg)
{
    nut_client_context_t* context = (nut_client_context_t*)arg;
    int client_fd = context->client_fd;
    struct sockaddr_in client_addr = context->client_addr;

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
    printf("Accepted connection from %s:%u\n", addr_str, ntohs(client_addr.sin_port));

    char buf[BUF_SIZE_READ];
    size_t buf_len = 0;

    while (1)
    {
        char* line;
        ssize_t n = nut_proto_readline(client_fd, buf, BUF_SIZE_READ, &buf_len, &line);

        if (n == 0)
        {
            // Socket closed.
            break;
        }

        if (n < 0)
        {
            break;
        }

        printf("[%s:%d] len: %2ld, data: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), n, line);

        int ret = nut_handle_command(context, line);
        if (ret < 0)
        {
            printf("Failed to handle command.\n");
            break;
        }

        if (ret == NUT_CMD_UNHANDLED)
        {
            printf("Unknown command.\n");

            nut_proto_writelinef(client_fd, "ERR UNKNOWN-COMMAND");
            break;
        }
    }

    printf("Client %s:%u disconnected\n", addr_str, ntohs(client_addr.sin_port));

    close(client_fd);
    free(context);
    return NULL;
}

nut_server_t* nut_server_create(const struct sockaddr_in* addr, const nut_ups_t* const* ups_devices, size_t ups_devices_count, const char* username, const char* password)
{
    // For simplicity, we only support a single admin user.

    if (!addr || !ups_devices || ups_devices_count == 0 || !username || !password)
    {
        return NULL;
    }

    nut_server_t* server = malloc(sizeof(nut_server_t));
    if (!server)
    {
        return NULL;
    }

    memcpy((void*)&server->addr, addr, sizeof(struct sockaddr_in));
    
    server->ups_devices = kv_store_init(NULL, 0);
    if (!server->ups_devices)
    {
        free(server);
        return NULL;
    }

    server->admin_username = strdup(username);
    server->admin_password = strdup(password);

    for (size_t i = 0; i < ups_devices_count; ++i)
    {
        const nut_ups_t* ups_device = ups_devices[i];
        kv_store_set(server->ups_devices, ups_device->name, ups_device);
    }

    return server;
}

static int destroy_ups(const kv_store_pair_t* pair, void* user_data)
{
    (void)user_data;

    nut_ups_t* const ups = (nut_ups_t*)pair->value;
    nut_ups_destroy(ups);
    
    return 0;
}

void nut_server_destroy(nut_server_t* server)
{
    if (!server)
    {
        return;
    }

    if (server->ups_devices)
    {
        kv_store_iterate(server->ups_devices, destroy_ups, NULL);
        kv_store_destroy(server->ups_devices);
    }

    if (server->admin_username)
    {
        free(server->admin_username);
    }

    if (server->admin_password)
    {
        free(server->admin_password);
    }

    free(server);
}

int nut_server_start(nut_server_t* server)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(sock);
        return -1;
    }

    if (bind(sock, (const struct sockaddr*)&server->addr, sizeof(server->addr)) < 0)
    {
        perror("bind");
        close(sock);
        return -1;
    }

    if (listen(sock, 16) < 0)
    {
        perror("listen");
        close(sock);
        return -1;
    }

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
    {
        perror("pthread_attr_init");
        close(sock);
        return -1;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        perror("pthread_attr_setdetachstate");
        pthread_attr_destroy(&attr);
        close(sock);
        return -1;
    }

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        nut_client_context_t* context = malloc(sizeof(nut_client_context_t));
        if (!context)
        {
            perror("malloc (connection context)");
            close(client_fd);
            continue;
        }

        context->server = server;
        context->client_fd = client_fd;
        context->client_addr = client_addr;
        context->username = NULL;
        context->password = NULL;

        pthread_t tid;
        if (pthread_create(&tid, &attr, nut_server_connection, context) != 0)
        {
            perror("pthread_create");
            close(client_fd);
            free(context);
            continue;
        }
    }

    printf("huh?\n");

    // Not reached, but for completeness.
    pthread_attr_destroy(&attr);
    close(sock);
    return 0;
}
