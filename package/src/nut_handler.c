#include "nut_handler.h"
#include "nut_proto.h"

#include <string.h>
#include <stdio.h>
#include <stddef.h>

// Handler function type: receives client_fd, (argc, argv) for arguments AFTER the command word.
typedef int (*nut_command_handler_fn)(nut_client_context_t* context, int argc, char **argv);

typedef struct nut_command_entry
{
    const char *cmd;
    nut_command_handler_fn handler;
} nut_command_entry_t;

// Tries to match argv[0] with a command in command_table, calls handler if found.
// Returns NUT_CMD_HANDLED, NUT_CMD_UNHANDLED, or < 0 on error.
static int dispatch(nut_client_context_t* context, int argc, char** argv, const nut_command_entry_t* command_table, size_t count)
{
    const char *cmd = argv[0];

    for (size_t i = 0; i < count; ++i)
    {
        if (strcmp(cmd, command_table[i].cmd) == 0)
        {
            return command_table[i].handler(context, argc - 1, argv + 1);
        }
    }

    return NUT_CMD_UNHANDLED;
}

typedef struct iter_result_arg
{
    int result;
    const void* user_data;
} iter_result_arg_t;

// TODO: ret == 0 => socket closed; should not indicate "unhandled".

#define WRITELINE(sock_fd, data) \
    ret = nut_proto_writeline((sock_fd), (data)); \
    if (ret <= 0) \
    { \
        return ret; \
    }

#define WRITELINEF(sock_fd, fmt, ...) \
    ret = nut_proto_writelinef((sock_fd), (fmt), __VA_ARGS__); \
    if (ret < 0) \
    { \
        return ret; \
    }

static int handle_starttls(nut_client_context_t* context, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret;
    WRITELINE(context->client_fd, "ERR FEATURE-NOT-SUPPORTED");

    return NUT_CMD_HANDLED;
}

static int handle_username(nut_client_context_t* context, int argc, char** argv)
{
    int ret;

    if (argc != 1)
    {
        WRITELINE(context->client_fd, "ERR INVALID-ARGUMENT");
        return -1;
    }

    if (context->username)
    {
        WRITELINE(context->client_fd, "ERR ALREADY-SET-USERNAME");
        return NUT_CMD_HANDLED;
    }

    context->username = argv[0];

    WRITELINE(context->client_fd, "OK");

    return NUT_CMD_HANDLED;
}

static int handle_password(nut_client_context_t* context, int argc, char** argv)
{
    int ret;

    if (argc != 1)
    {
        WRITELINE(context->client_fd, "ERR INVALID-ARGUMENT");
        return -1;
    }

    if (!context->username)
    {
        WRITELINE(context->client_fd, "ERR USERNAME-REQUIRED");
        return NUT_CMD_HANDLED;
    }

    if (context->password)
    {
        WRITELINE(context->client_fd, "ERR ALREADY-SET-PASSWORD");
        return NUT_CMD_HANDLED;
    }

    context->password = argv[0];

    WRITELINE(context->client_fd, "OK");

    return NUT_CMD_HANDLED;
}

static int handle_login(nut_client_context_t* context, int argc, char** argv)
{
    int ret;

    if (argc != 1)
    {
        WRITELINE(context->client_fd, "ERR INVALID-ARGUMENT");
        return -1;
    }

    const char* ups_name = argv[0];

    const kv_store_pair_t* ups_device_entry = kv_store_get(context->server->ups_devices, ups_name);
    if (!ups_device_entry)
    {
        WRITELINE(context->client_fd, "ERR UNKNOWN-UPS");
        return NUT_CMD_HANDLED;
    }

    // TODO: Maintain attach state per UPS.

    WRITELINE(context->client_fd, "OK");

    return NUT_CMD_HANDLED;
}

static int handle_logout(nut_client_context_t* context, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // No error handling. The client might have closed the connection already at this point.
    nut_proto_writeline(context->client_fd, "OK Goodbye;");

    return NUT_CMD_HANDLED;
}

static int print_ups(const kv_store_pair_t* pair, void* user_data)
{
    iter_result_arg_t* result_arg = (iter_result_arg_t*)user_data;
    const nut_client_context_t* context = (const nut_client_context_t*)result_arg->user_data;

    const nut_ups_t* ups = (const nut_ups_t*)pair->value;

    int ret = nut_proto_writelinef(context->client_fd, "UPS %s \"%s\"", ups->name, ups->description);
    if (ret <= 0)
    {
        result_arg->result = ret;
        return -1;
    }
    
    return 0;
}

static int handle_list_ups(nut_client_context_t* context, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int ret;
    WRITELINE(context->client_fd, "BEGIN LIST UPS");

    iter_result_arg_t result_arg = { 0, context };
    kv_store_iterate(context->server->ups_devices, print_ups, &result_arg);
    if (result_arg.result < 0)
    {
        return result_arg.result;
    }

    WRITELINE(context->client_fd, "END LIST UPS");

    return NUT_CMD_HANDLED;
}

typedef struct print_var_iter_args
{
    const nut_client_context_t* context;
    const char* ups_name;
} print_var_iter_args_t;

static int print_var(const kv_store_pair_t* pair, void* user_data)
{
    iter_result_arg_t* result_arg = (iter_result_arg_t*)user_data;
    const print_var_iter_args_t* args = (const print_var_iter_args_t*)result_arg->user_data;
    const nut_client_context_t* context = args->context;
    const char* ups_name = args->ups_name;

    const nut_ups_variable_t* var = (const nut_ups_variable_t*)pair->value;

    int ret;
    if (var->type == NUT_VAR_TYPE_STRING)
    {
        ret = nut_proto_writelinef(context->client_fd, "VAR %s %s \"%s\"", ups_name, var->name, var->value);
    }
    else
    {
        ret = nut_proto_writelinef(context->client_fd, "VAR %s %s %s", ups_name, var->name, var->value);
    }

    if (ret <= 0)
    {
        result_arg->result = ret;
        return -1;
    }
    
    return 0;
}

static int handle_list_var(nut_client_context_t* context, int argc, char** argv)
{
    int ret;

    if (argc != 1)
    {
        WRITELINE(context->client_fd, "ERR INVALID-ARGUMENT");
        return -1;
    }

    const char* ups_name = argv[0];

    const kv_store_pair_t* ups_device_entry = kv_store_get(context->server->ups_devices, ups_name);
    if (!ups_device_entry)
    {
        WRITELINE(context->client_fd, "ERR UNKNOWN-UPS");
        return NUT_CMD_HANDLED;
    }

    const nut_ups_t* ups_device = (const nut_ups_t*)ups_device_entry->value;

    WRITELINEF(context->client_fd, "BEGIN LIST VAR %s", ups_name);
    
    const print_var_iter_args_t args = { context, ups_name };
    iter_result_arg_t result_arg = { 0, &args };
    kv_store_iterate(ups_device->ups_variables, print_var, &result_arg);
    if (result_arg.result < 0)
    {
        return result_arg.result;
    }
    
    WRITELINEF(context->client_fd, "END LIST VAR %s", ups_name);

    return NUT_CMD_HANDLED;
}

static int handle_list(nut_client_context_t* context, int argc, char** argv)
{
    static const nut_command_entry_t command_table[] = 
    {
        { "UPS", handle_list_ups },
        { "VAR", handle_list_var }
    };

    return dispatch(context, argc, argv, command_table, sizeof(command_table) / sizeof(command_table[0]));
}

static int handle_get_var(nut_client_context_t* context, int argc, char** argv)
{
    int ret;

    if (argc != 2)
    {
        WRITELINE(context->client_fd, "ERR INVALID-ARGUMENT");
        return -1;
    }

    const char* ups_name = argv[0];
    const char* var_name = argv[1];

    const kv_store_pair_t* ups_device_entry = kv_store_get(context->server->ups_devices, ups_name);
    if (!ups_device_entry)
    {
        WRITELINE(context->client_fd, "ERR UNKNOWN-UPS");
        return NUT_CMD_HANDLED;
    }

    const nut_ups_t* ups_device = (const nut_ups_t*)ups_device_entry->value;

    const kv_store_pair_t* var_entry = kv_store_get(ups_device->ups_variables, var_name);
    if (var_entry == NULL)
    {
        WRITELINE(context->client_fd, "ERR VAR-NOT-SUPPORTED");
        return NUT_CMD_HANDLED;
    }

    const nut_ups_variable_t* var = (const nut_ups_variable_t*)var_entry->value;

    if (var->type == NUT_VAR_TYPE_STRING)
    {
        ret = nut_proto_writelinef(context->client_fd, "VAR %s %s \"%s\"", ups_name, var->name, var->value);
    }
    else
    {
        ret = nut_proto_writelinef(context->client_fd, "VAR %s %s %s", ups_name, var->name, var->value);
    }

    return NUT_CMD_HANDLED;
}

static int handle_get(nut_client_context_t* context, int argc, char** argv)
{
    static const nut_command_entry_t command_table[] = 
    {
        { "VAR", handle_get_var }
    };

    return dispatch(context, argc, argv, command_table, sizeof(command_table) / sizeof(command_table[0]));
}

static int handle_main(nut_client_context_t* context, int argc, char** argv)
{
    static const nut_command_entry_t command_table[] = 
    {
        { "STARTTLS", handle_starttls },
        { "USERNAME", handle_username },
        { "PASSWORD", handle_password },
        { "LOGIN", handle_login },
        { "ATTACH", handle_login },
        { "LOGOUT", handle_logout },
        { "DETACH", handle_logout },
        { "GET", handle_get },
        { "LIST", handle_list }
    };

    return dispatch(context, argc, argv, command_table, sizeof(command_table) / sizeof(command_table[0]));
}

#define NUT_MAX_ARGS 16

int nut_handle_command(nut_client_context_t* context, char* command)
{
    if (command == NULL)
    {
        return NUT_CMD_ERROR;
    }

    char* argv[NUT_MAX_ARGS];
    int argc = nut_proto_split_args(command, argv, NUT_MAX_ARGS);
    if (argc <= 0)
    {
        printf("Failed to split NUT command.\n");
        return NUT_CMD_ERROR;
    }

    return handle_main(context, argc, argv);
}
