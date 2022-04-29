#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int queue_init(queue_t *queue) {
    queue->head = NULL;
    queue->size = 0;
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

int queue_add(queue_t *queue, const char *item) {
    // Initialize the int for error checking
    int result;
    // Attempt to lock the region of code. If it throws an error, print the error.
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }
    // Check to see if the queue is full. If it is, setup a pthread_cond_wait for the queue to be full, which also touched the lock for some reason.
    while (queue->size == MAX_QUEUE_SIZE) {
        if ((result = pthread_cond_wait(&queue->queue_full, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            return -1;
        }
    }

    // This stuff is explicitly related to the lab, I'm pretty sure. I don't think that there's anything here that's particularly useful, besides the structure of the loops and the lines concerning the mutex locking and unlocking.
    if (queue->head == NULL) {
        if ((queue->head = malloc(sizeof(node_t))) == NULL) {
            fprintf(stderr, "malloc() failed\n");
            // If there is an error, unlock the mutex. VERY IMPORTANT.
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        strncpy(queue->head->message, item, BUFLEN - 1);
        queue->head->message[BUFLEN - 1] = '\0';
        queue->head->next = NULL;
        queue->size = 1;
    } else {
        node_t *current = queue->head;
        while (current->next != NULL) {
            current = current->next;
        }
        if ((current->next = malloc(sizeof(node_t))) == NULL) {
            fprintf(stderr, "malloc() failed\n");
            // Same as above. If there is an error detected at any point, return with an error and unlock the mutex.
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        strncpy(current->next->message, item, BUFLEN - 1);
        current->next->message[BUFLEN - 1] = '\0';
        current->next->next = NULL;
        queue->size++;
    }

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

int queue_remove(queue_t *queue, char *item_dest) {
    int result;
    if ((result = pthread_mutex_lock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(result));
        return -1;
    }

    while (queue->size == 0) {
        if ((result = pthread_cond_wait(&queue->queue_empty, &queue->lock)) != 0) {
            fprintf(stderr, "pthread_cond_wait: %s\n", strerror(result));
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
    }

    strcpy(item_dest, queue->head->message);
    node_t *new_head = queue->head->next;
    free(queue->head);
    queue->head = new_head;
    queue->size--;

    if ((result = pthread_cond_signal(&queue->queue_full)) != 0) {
        fprintf(stderr, "pthread_cond_signal: %s\n", strerror(result));
        return -1;
    }
    if ((result = pthread_mutex_unlock(&queue->lock)) != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(result));
        return -1;
    }

    return 0;
}
