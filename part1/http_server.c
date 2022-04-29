#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

// Makes a TCP server
// host: Name of server to connect to
// port: Port to connect to
// Returns a new TCP socket file descriptor on success or -1 on error
int open_server(const char *host, const char *port) {
    // TODO Not yet implemented

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *server;
    int ret_val = getaddrinfo(host, port, &hints, &server);
    if(ret_val != 0){
        printf("getaddrinfo failed: %s\n", gai_strerror(ret_val));
    }
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if(sock_fd == -1){
        perror("socket failure:\n");
    }
    connect(sock_fd, server->ai_addr, server->ai_addrlen);
    return sock_fd;
}



int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions:
    const char *serve_dir = argv[1];
    const char *port = argv[2];

    // TODO Complete the rest of this function
    printf("Making hints...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server; 
    printf("Getting address info...\n");
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val !=0){
        printf("getaddrinfo failure %s\n", gai_strerror(ret_val));
    }
    printf("Opening socket...\n");
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1){
        perror("socket failure: \n");
    }
    printf("Binding...\n");
    bind(sock_fd, server->ai_addr, server->ai_addrlen);
    printf("Listening...\n");
    listen(sock_fd, LISTEN_QUEUE_LEN);
    // Below here we start looping
    int stoploop = 0;
    while(stoploop == 0){
        // Does this even do anything? I certainly don't know.
        printf("Accepting...\n");
        int newsock = accept(sock_fd, server->ai_addr, &server->ai_addrlen);
        char mybuffer[BUFSIZE];
        char fullpath[BUFSIZE];
        printf("Reading HTTP request...\n");
        read_http_request(newsock, mybuffer);
        strcpy(fullpath, "./");
        strcat(fullpath, serve_dir);
        strcat(fullpath, "/");
        strcat(fullpath, mybuffer);
        printf("Writing HTTP response...\n");
        write_http_response(newsock, fullpath); 
        close(newsock);
    }
    return 0;
}
