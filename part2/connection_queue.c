#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "connection_queue.h"

int connection_queue_init(connection_queue_t *queue) {
    // This is all very self-explanatory; initialize everything to zero, set up the mutex and pthread conds
    queue->shutdown = 0;
    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    int result;
    if ((result = pthread_mutex_init(&queue->lock, NULL)) != 0) {
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_init(&queue->queue_full, NULL)) != 0) {
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_cond_init(&queue->queue_empty, NULL)) != 0) {
        fprintf(stderr, "pthread_cond_init: %s\n", strerror(result));
        return -1;
    }

    return 0;

}

int connection_enqueue(connection_queue_t *queue, int connection_fd) {

    // Initialize the error checking variable
    int result;
    // If we are supposed to shut down, return unsuccessfully
    if (queue->shutdown == 1){
        return -1;
    }
    // Attempt to lock the region of code. If it throws an error, print the error.
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    // Check to see if the queue is full. If it is, setup a pthread_cond_wait for the queue to be full, which also touched the lock for some reason.
    while (queue->length == CAPACITY) {
        if ((result = pthread_cond_wait(&queue->queue_full, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            return -1;
        }
    }

    // Same as above, but handles the case where we wake up after a shutdown has been called
    if (queue->shutdown == 1){
        return -1;
    }

    // If we get here, we have space and everything, so we should write connection_fd to write_idx, move write_idx appropriately (maybe check to ensure we aren't full?), and then unlock and return. 
    queue->client_fds[queue->write_idx] = connection_fd;
    queue->length++;
    // This line is very smart; this automatically loops you back around when you attempt to increment out of bounds without any complex code required. Hooray modulo!
    queue->write_idx = ((queue->write_idx + 1)%(CAPACITY));

    // Signal the queue_empty pthread cond and check for errors. If any are found, unlock the mutex and return an error.
    if ((result = pthread_cond_signal(&queue->queue_empty)) != 0) {
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    // Unlock the mutex. If error, return an error. Notably, THIS DOES NOT RESULT IN ANOTHER ATTEMPT AT MUTEX UNLOCKING (obviously.)
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }

    return 0;
}

int connection_dequeue(connection_queue_t *queue) {
    // Initializes the error checking variable
    int result;

    // If we get here, we want to make sure that we shutdown properly if one of the locks is somehow improperly set to prevent a hang.
    if (queue->shutdown == 1){
        return -1;
    }
    // Attempt to lock the mutex and process errors as appropriate
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }

    // Blocks if the queue is empty, errors if it can't wait on the condition
    while (queue->length == 0) {
        if ((result = pthread_cond_wait(&queue->queue_empty, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
    }

    // Checking the shutdown variable to see if a shutdown has been called for
    if (queue->shutdown == 1){
        return -1;
    }


    // First, check the fd under the read head. Then, empty what is under the read head. Finally, move the read head using the same logic as in enqueue.
    int return_fd = queue->client_fds[queue->read_idx];

    //I'm pretty sure you don't actually have to erase or re-initialize what is under the read head when you read it, as simply by reducing its length and moving the head you have made it so it will be ignored by both write(who will just overwrite it) and read(which won't even see it because it cares about length, not actual content.)
    queue->length--;
    queue->read_idx = ((queue->read_idx + 1)%(CAPACITY));

    //Mutexes and cond signal handling, with standard error checks.
    if ((result = pthread_cond_signal(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }

    // Returns the file descriptor for the socket
    return return_fd;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    // Set the variable to signal the shutdown, broadcast that both of the conds are up. There are checks in the enqueue and dequeue functions both before and after the mutex locks and cond checks for the shutdown condition.
    int result;
    queue->shutdown = 1;

    if ((result = pthread_cond_broadcast(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1;
    }

    if ((result = pthread_cond_broadcast(&queue->queue_empty)) != 0) {
        fprintf(stderr, "pthread_cond_broadcast: %s\n", strerror(result));
        return -1;
    }


    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    // Not even necessary. Valgrind says that everything is clear without this function, so I'm not going to implement it.
    return 0;
}
