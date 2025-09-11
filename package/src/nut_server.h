#ifndef NUT_SERVER_H
#define NUT_SERVER_H

#include "kv_store.h"

#include <stdint.h>
#include <netinet/in.h>
#include <stddef.h>

typedef enum nut_variable_type {
    NUT_VAR_TYPE_STRING,
    NUT_VAR_TYPE_NUMBER
} nut_variable_type_t;

typedef struct nut_ups_variable
{
    const char* name;
    nut_variable_type_t type;
    const char* value;
} nut_ups_variable_t;

typedef struct nut_ups {
    char* name;
    char* description;
    kv_store_t* ups_variables;
} nut_ups_t;

nut_ups_t* nut_ups_create(const char* name, const char* description, const nut_ups_variable_t* ups_variables, size_t ups_variables_count);

void nut_ups_destroy(nut_ups_t* ups);

typedef struct nut_server {
    const struct sockaddr_in addr;
    kv_store_t* ups_devices;
    char* admin_username;
    char* admin_password;
} nut_server_t;

// Create a new nut_server_t instance with the given address and array of UPS devices.
// nut_server_destroy will automatically destroy all nut_ups_t* passed in ups_devices.
nut_server_t* nut_server_create(const struct sockaddr_in* addr, const nut_ups_t* const* ups_devices, size_t ups_devices_count, const char* admin_username, const char* admin_password);

// Start the server listener for the given server instance.
// Returns 0 on success, nonzero on error.
int nut_server_start(nut_server_t* server);

// Frees all resources associated with the server.
void nut_server_destroy(nut_server_t* server);

#endif // NUT_SERVER_H
