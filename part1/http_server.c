#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

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
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *server; 
    int ret_val = getaddrinfo(host, port, &hints, &server);
    if (ret_val !=0){
        printf("getaddrinfo failure %s\n", gai_strerror(ret_val));
    }
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1){
        perror("socket failure: \n");
    }
    bind(sock_fd, )
    
    return 0;
}
