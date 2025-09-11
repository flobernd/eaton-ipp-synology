#ifndef NUT_PROTO_H
#define NUT_PROTO_H

#include <stddef.h>
#include <sys/types.h>

#define BUF_SIZE_WRITE 1024

// Splits arguments according to the NUT protocol specification, removes escape chars from output args.
// Modifies the input string.
// Returns -1 if max_args would be exceeded, else argc.
int nut_proto_split_args(char *command, char **argv_out, int max_args);

// Reads a line from the socket into buf, null-terminates, and outputs a pointer to the line.
// Returns the length of the line, 0 on EOF, or -1 on error.
ssize_t nut_proto_readline(int sock_fd, char *buf, size_t buf_size, size_t *buf_len, char **line_out);

// Writes a line to the socket. Returns bytes written or -1 on error.
int nut_proto_writeline(int sock_fd, const char *line);

// Writes a formatted line to the socket. Returns bytes written or -1 on error.
int nut_proto_writelinef(int sock_fd, const char *fmt, ...);

#endif // NUT_PROTO_H
