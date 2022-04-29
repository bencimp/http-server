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
        printf("txt\n");
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        printf("html\n");
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        printf("jpg\n");
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        printf("png\n");
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        printf("pdf\n");
        return "application/pdf";
    }

    return NULL;
}

// int read_http_request(int fd, char *resource_name) {
    // TODO Not yet implemented
    // return 0;
// }

int write_http_response(int fd, const char *resource_path) {
    // TODO Not yet implemented

    char error404[] = "HTTP/1.0 404 Not Found\r\n";
    // FIRST THING, I need to check to make sure the file actually exists. Conveniently, stat() got my back.
    printf("Resource path: %s\n", resource_path);

    struct stat mystatbuf; 
    int filelength = stat(resource_path, &mystatbuf);
    if (filelength == -1){
        printf("File Not Found.\n");
        write(fd, error404, strlen(error404));
        return -1;
    }
    //Below is the response header

    
    // char filext[5];
    const char *dot = strrchr(resource_path, '.');
    if(!dot || dot == resource_path){
        // Ditto as above
        write(fd, error404, strlen(error404));
        return -1;
    }

    // return dot + 1;

    // This marks the end of the response headers and beginning of actual content
    char buf[BUFSIZE];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    strcat(buf, "Content-Type: ");

    printf("Size from stat: %ld\n", mystatbuf.st_size);
    // THIS LINE NEEDS REVISION PENDING EXTENSION EXTRACTOR CODE
    strcat(buf, get_mime_type(dot));
    strcat(buf, "\r\nContent-Length: ");

    char sizebuf[12];
    // THIS LINE NEEDS REVISION PENDING LENGTH CODE
    // strcat(buf, itoa(mystatbuf.st_size));

    // Below here is the actual code to write the file out. May need to shift some of this upwards as to have an easy way to calculate file length.

    FILE *myFile = fopen(resource_path, "rb");
    int bytes_read;
    fseek(myFile, 0, SEEK_END);
    sprintf(sizebuf, "%ld", ftell(myFile));
    printf("Size buffer: %ld\n", ftell(myFile));
    strcat(buf, sizebuf);
    strcat(buf, "\r\n\r\n");
    printf("Request:\n%s\n", buf);
    write(fd, buf, strlen(buf));
    fseek(myFile, 0, SEEK_SET);


    while((bytes_read = fread(buf, 1, BUFSIZE, myFile)) > 0){
        if (write(fd, buf, bytes_read) == -1) {
            perror("write");
            close(fd);
            return -1;
        }
    }
    if (bytes_read == -1) {
        perror("read");
        return -1;
    }


    return 0;


    // Open destination file for writing
    // int file_fd = open(resource_path, O_RDONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    // if (file_fd == -1) {
    //     perror("open");
    //     return -1;
    // }
    // // weewooweewoo
    // int bytes_read;
    // while ((bytes_read = read(file_fd, buf, BUFSIZE)) > 0) {
    //     if (write(fd, buf, bytes_read) == -1) {
    //         perror("write");
    //         close(fd);
    //         return -1;
    //     }
    // }
    // if (bytes_read == -1) {
    //     perror("read");
    //     close(file_fd);
    //     return -1;
    // }

    // if (close(file_fd) != 0) {
    //     perror("close");
    //     return -1;
    // }

    // return 0;
}

int read_http_request(int fd, char *resource_name) {
    // // stdio FILE * gives us 'fgets()' to easily read line by line
    // int sock_fd_copy = dup(fd);
    // if (sock_fd_copy == -1) {
    //     perror("dup");
    //     return -1;
    // }
    // FILE *socket_stream = fdopen(sock_fd_copy, "r");
    // if (socket_stream == NULL) {
    //     perror("fdopen");
    //     close(sock_fd_copy);
    //     return -1;
    // }
    // // Disable the usual stdio buffering
    // if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
    //     perror("setvbuf");
    //     fclose(socket_stream);
    //     return -1;
    // }
    
    char buffer[BUFSIZE];
    // char pasbuf[BUFSIZE];
    read(fd, buffer, sizeof(buffer));
    char *tempbuf = strchr(buffer, '/');
    char *eventemperbuf = strchr(tempbuf, ' ');
    eventemperbuf[0] = '\0';
    printf("Pass Buffer: \"%s\"\n", tempbuf);
    // resource_name = tempbuf;
    strcpy(resource_name, tempbuf+1);
    // strcpy(resource_name, pasbuf);

    return 0;
}

