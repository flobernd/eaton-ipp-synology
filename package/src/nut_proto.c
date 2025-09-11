#include "nut_proto.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

int nut_proto_split_args(char *command, char **argv, int max_args)
{
    if (!command || !argv || max_args <= 0) 
    {
        return 0;
    }

    int argc = 0;
    char *p = command;

    // Skip initial whitespaces.
    while (*p && isspace((unsigned char)*p))
    {
        ++p;
    } 

    while (*p && argc < max_args)
    {
        if (*p == '"')
        {
            // Quoted token: copy into place, handling escapes.
            ++p; // consume opening quote.
            char *w = p;
            argv[argc++] = w;

            while (*p)
            {
                char c = *p++;
                if (c == '\\')
                {
                    char next = *p;
                    if (next == '"' || next == '\\')
                    {
                        *w++ = next;
                        ++p; // consume escaped char
                    }
                    else
                    {
                        // Keep backslash literally if not escaping '"' or '\'.
                        *w++ = '\\';
                    }
                }
                else if (c == '"')
                {
                    // End of quoted token
                    break;
                }
                else
                {
                    *w++ = c;
                }
            }

            *w = '\0';

            // After a quoted token, leave p at the character after the closing quote (if any).
            // Next loop will either start a new token or skip whitespace below.
        }
        else
        {
            // Unquoted token: don't copy, just terminate in place.
            argv[argc++] = p;
            while (*p && !isspace((unsigned char)*p))
            {
                ++p;
            }

            // If we stopped on whitespace, terminate token and advance past the first delimiter.
            if (*p)
            {
                *p++ = '\0';
            }
        }

        // Skip whitespace before next token.
        while (*p && isspace((unsigned char)*p))
        {
            ++p;
        }
    }

    return argc;
}

ssize_t nut_proto_readline(int sock_fd, char *buf, size_t buf_size, size_t *buf_len, char **line_out)
{
    ssize_t ret;
    size_t len = *buf_len;

    for (;;)
    {
        char *newline = memchr(buf, '\n', len);
        if (newline)
        {
            size_t line_len = (size_t)(newline - buf);
            // Null-terminate the line.
            buf[line_len] = '\0';
            *line_out = buf;
            size_t remain = len - (line_len + 1);
            if (remain > 0)
            {
                memmove(buf, newline + 1, remain);
            }

            *buf_len = remain;
            return (ssize_t)line_len;
        }

        if (len == buf_size)
        {
            *buf_len = 0;
            return -1;
        }

        ret = read(sock_fd, buf + len, buf_size - len);
        if (ret < 0)
        {
            perror("read");
            return -1;
        }

        if (ret == 0)
        {
            return 0;
        }

        len += ret;
        *buf_len = len;
    }
}

int nut_proto_writeline(int sock_fd, const char *line)
{
    size_t len = strlen(line);
    ssize_t total_written = 0;
    ssize_t written;

    // Write the data string.
    while ((size_t)total_written < len)
    {
        written = write(sock_fd, line + total_written, len - total_written);
        if (written < 0)
        {
            perror("write");
            return -1;
        }

        if (written == 0)
        {
            return 0;
        }

        total_written += written;
    }

    // Write the newline character.
    written = write(sock_fd, "\n", 1);
    if (written < 0)
    {
        perror("write");
        return -1;
    }

    return written;
}

int nut_proto_writelinef(int sock_fd, const char *fmt, ...)
{
    char buffer[BUF_SIZE_WRITE];
    va_list args;
    int len;
    ssize_t total_written = 0;
    ssize_t written;

    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0)
    {
        return -1;
    }

    if ((size_t)len >= sizeof(buffer))
    {
        return -1;
    }

    // Write the formatted string.
    while (total_written < len)
    {
        written = write(sock_fd, buffer + total_written, len - total_written);
        if (written < 0)
        {
            perror("write");
            return -1;
        }

        if (written == 0)
        {
            return 0;
        }

        total_written += written;
    }

    // Write the newline character.
    written = write(sock_fd, "\n", 1);
    if (written < 0)
    {
        perror("write");
        return -1;
    }

    return written;
}
