#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "http.h"

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    // TODO Not yet implemented
    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    // TODO Not yet implemented
    return 0;
}

int read_http_response(int sock_fd, const char *file) {
    // stdio FILE * gives us 'fgets()' to easily read line by line
    int sock_fd_copy = dup(sock_fd);
    if (sock_fd_copy == -1) {
        perror("dup");
        return -1;
    }
    FILE *socket_stream = fdopen(sock_fd_copy, "r");
    if (socket_stream == NULL) {
        perror("fdopen");
        close(sock_fd_copy);
        return -1;
    }
    // Disable the usual stdio buffering
    if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        fclose(socket_stream);
        return -1;
    }

    // Keep consuming lines until we find an empty line
    // This marks the end of the response headers and beginning of actual content
    char buf[BUFSIZE];
    while (fgets(buf, BUFSIZE, socket_stream) != NULL) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
    }

    if (fclose(socket_stream) != 0) {
        perror("fclose");
        return -1;
    }

    // Open destination file for writing
    int file_fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if (file_fd == -1) {
        perror("open");
        return -1;
    }

    int bytes_read;
    while ((bytes_read = read(sock_fd, buf, BUFSIZE)) > 0) {
        if (write(file_fd, buf, bytes_read) == -1) {
            perror("write");
            close(file_fd);
            return -1;
        }
    }
    if (bytes_read == -1) {
        perror("read");
        close(file_fd);
        return -1;
    }

    if (close(file_fd) != 0) {
        perror("close");
        return -1;
    }

    return 0;
}

