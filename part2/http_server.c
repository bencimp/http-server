#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

// #defining debug as one is one of the ways I usually test, it's set to zero for the versions of programs being turned in.
#define debug 0
#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

// Information passed to consumer threads
typedef struct {
    int idx;
    connection_queue_t *queue;
} thread_args_t;

int keep_going = 1;
const char *serve_dir;


void handle_sigint(int signo) {
    keep_going = 0;
}


// The function that the worker threads use. Very simple, just a loop that gets file descriptors for connections, reads the request, writes the response, and closes the connection. Takes in a void pointer that is later cast to a thread_args_t, should not ever return, only exit(1).
void *worker_thread_func(void *arg){
    // Cast the void pointer into a thread_args_t, which allows us to access the queue
    thread_args_t *args = (thread_args_t *) arg;
    int socket_fd = 0;
    while(keep_going){
        // Here, we do a lot of the work that we used to do in the main function.
        // In order, we will:
        // attempt to read a socket file descriptor using connection_dequeue. This will block if the queue is empty, or get us the number successfully if it doesn't. 
        if((socket_fd = connection_dequeue(args->queue)) == -1){
            exit(1);
        }


        char mybuffer[BUFSIZE];
        char fullpath[BUFSIZE];
        if(debug) printf("Reading HTTP request...\n");
        // Then, we will read the http request from the socket into a buffer. The buffer will be initialized after the thread unblocks for getting the socket file descriptor.
        read_http_request(socket_fd, mybuffer);
        strcpy(fullpath, "./");
        strcat(fullpath, serve_dir);
        strcat(fullpath, "/");
        strcat(fullpath, mybuffer);
        if(debug) printf("Writing HTTP response...\n");
        // Then, we will write the http response and close the socket. Simplicity itself.
        write_http_response(socket_fd, fullpath); 
        close(socket_fd);
    }
    return 0;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    serve_dir = argv[1];

    const char *port = argv[2];

    // Initialize the connection queue.
    connection_queue_t queue;
    if (connection_queue_init(&queue) != 0){
        fprintf(stderr, "Failed to initialize queue\n");
        return 1;
    }

    // Boilerplate for opening up the TCP socket.
    if(debug) printf("Making hints...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server; 
    if(debug) printf("Getting address info...\n");
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val !=0){
        printf("getaddrinfo failure %s\n", gai_strerror(ret_val));
    }
    if(debug) printf("Opening socket...\n");
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1){
        perror("socket failure: \n");
    }
    if(debug) printf("Binding...\n");
    if(bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1){
        perror("bind: \n");
        return -1;
    }
    if(debug) printf("Listening...\n");
    if(listen(sock_fd, LISTEN_QUEUE_LEN) == -1){
        perror("listen: \n");
        return -1;
    }

    // Error handling variable.
    int result;
    
    // Before threads are spawned, use sigprocmask to ignore all signals. Check for errors on all calls.
    struct sigaction sact;
    sigset_t oldset, newset;
    if(sigemptyset(&sact.sa_mask) == -1){
        perror("sigemptyset: \n");
        return -1;
    }    
    sact.sa_flags = 0;
    sact.sa_handler = handle_sigint;
    if(sigaction(SIGINT, &sact, NULL) == -1){
        perror("sigaction: \n");
        return -1;
    }
    if(sigfillset(&newset) == -1){
        perror("sigfillset: \n");
        return -1;
    }
    if(sigprocmask(SIG_SETMASK, &newset, &oldset) == -1){
        perror("sigprocmask: \n");
        return -1;
    }

    // Initialize the worker threads. By default, there are five.
    pthread_t worker_threads[N_THREADS];
    thread_args_t thread_args[N_THREADS];
    for (int i = 0; i < N_THREADS; i ++){
        thread_args[i].queue = &queue;
        thread_args[i].idx = i;
        if ((result = pthread_create(worker_threads + i, NULL, worker_thread_func, thread_args + i) != 0)){
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            return 1;
        }

    }

    // Reset the old signal mask.
    if(sigprocmask(SIG_SETMASK, &oldset, NULL) == -1){
        perror("sigprocmask: \n");
        return -1;
    }

    // Loop to accept the connections. Since keep_going is a variable set by the mask for SIGINT, this basically says "keep going until I tell you to stop."
    while(keep_going){
        if(debug) printf("Accepting...\n");
        // More boilerplate. Accept the connection.
        int newsock = accept(sock_fd, server->ai_addr, &server->ai_addrlen);
        // Call enqueue. This will block if there's not enough space.
        connection_enqueue(&queue, newsock);
    }
    // If we are out of the loop, it's because keep_going has been set to zero, meaning it's time to clean up and stop running. Run the shutdown function, wait for the worker threads to stop.
    connection_queue_shutdown(&queue);
    for(int i = 0; i < N_THREADS; i ++){
        if ((result = pthread_join(worker_threads[i], NULL))!=0){
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
        }
    }
    return 0;
}
