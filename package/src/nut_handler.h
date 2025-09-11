#ifndef NUT_HANDLER_H
#define NUT_HANDLER_H

#include "nut_client_context.h"

#define NUT_CMD_UNHANDLED  0
#define NUT_CMD_HANDLED    1
#define NUT_CMD_ERROR     -1

// Parses and handles a given NUT protocol command line.
// Returns NUT_CMD_HANDLED if the command was recognized and handled successfully,
// NUT_CMD_UNHANDLED if the command is unknown, or a number < 0 on error.
// Modifies the input buffer.
int nut_handle_command(nut_client_context_t* context, char* input_line);

#endif // NUT_HANDLER_H
