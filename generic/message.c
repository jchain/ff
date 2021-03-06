#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "message.h"
#include "macros.h"

// C standard library
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX C library
#include <pthread.h>
#include <semaphore.h>

// Message container

struct _message {
    void (*freefn)(void *);
    void *data;
};

message *message_new(void *data, void (*freefn)(void *)) {
    message *msg = (message *)malloc(sizeof(message));
    msg->freefn = freefn;
    msg->data = data;
    return msg;
}

void *message_data(message *msg) { return msg->data; }

void message_free(message *msg) {
    if (msg == NULL) {
        return;
    }
    msg->freefn(msg->data);
    free(msg);
    msg = NULL;
}

// Message queue

typedef struct _node node;
struct _node {
    size_t priority;
    message *msg;
    node *next;
};

struct _queue {
    sem_t length;
    node *head;
    node *tail;
    pthread_mutex_t lock;
};

queue *queue_new() {
    queue *q = (queue *)malloc(sizeof(queue));
    sem_init(&q->length, 0, 0);
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->lock, NULL);
    return q;
}

void queue_free(queue *q) {
    if (q == NULL) {
        return;
    }
    sem_destroy(&q->length);
    pthread_mutex_destroy(&q->lock);
    free(q);
}

void queue_put(queue *q, message *msg, size_t priority) {
    node *new_node = (node *)malloc(sizeof(node));
    new_node->priority = priority;
    new_node->msg = msg;
    new_node->next = NULL;

    with_pthread_mutex(&q->lock) {
        if (q->head == NULL && q->tail == NULL) {
            // If the list was empty, create the first node
            q->head = new_node;
            q->tail = new_node;
        } else {
            // Walk the list until an item with lower priority is found or
            // the list has ended
            node *p = q->head;
            while (p != q->tail && p->priority > priority) {
                if (p->next == NULL) {
                    break;
                }
                p = p->next;
            }

            // Insert new item
            node *next = p->next;
            p->next = new_node;
            new_node->next = next;
        }
    }

    sem_post(&q->length);
}

void queue_put_head(queue *q, message *msg) {
    node *new_node = (node *)malloc(sizeof(node));
    new_node->priority = QUEUE_PRIORITY_MAX;
    new_node->msg = msg;
    new_node->next = NULL;

    with_pthread_mutex(&q->lock) {
        if (q->head == NULL && q->tail == NULL) {
            // If the list was empty, create the first node
            q->head = new_node;
            q->tail = new_node;
        } else {
            // next of the new node will be the current head
            new_node->next = q->head;
            q->head = new_node;
        }
    }

    sem_post(&q->length);
}

void queue_put_tail(queue *q, message *msg) {
    node *new_node = (node *)malloc(sizeof(node));
    new_node->priority = QUEUE_PRIORITY_MIN;
    new_node->msg = msg;
    new_node->next = NULL;

    with_pthread_mutex(&q->lock) {
        if (q->head == NULL && q->tail == NULL) {
            // If the list was empty, create the first node
            q->head = new_node;
            q->tail = new_node;
        } else {
            // next of the current tail will be the new tail
            q->tail->next = new_node;
            q->tail = new_node;
        }
    }

    sem_post(&q->length);
}

message *queue_get(queue *q) {
    // Wait until the queue is non-empty
    sem_wait(&q->length);

    message *msg = NULL;
    with_pthread_mutex(&q->lock) {
        // Retrieve the message and free the node
        msg = q->head->msg;
        node *oldhead = q->head;
        if (q->head->next) {
            // next is new head
            q->head = q->head->next;
        } else {
            // if there is no next, queue is empty
            q->head = NULL;
            q->tail = NULL;
        }
        free(oldhead);
    }

    return msg;
}
