/*
 * Code courtesy of Kristjon from plenaries:
 * https://github.com/kr1stj0n/plenaries-in3230-in4230-h22/tree/main/p7_28-10-2022/libqueue
 *
 * Changes:
 * - Removed `id` field. It seemingly served no function?
 * - Linux kernel style intentation.
 * - Added `queue_remove()` which selectivly removes an entry.
 * - Modified the "pop" functions to use `queue_remove()`.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <sys/types.h>

#define MAX_QUEUE_SIZE 16

struct queue_entry {
        struct queue_entry *next;
        struct queue_entry *prev;
        void *data;
};

struct queue {
        struct queue_entry *head;
        struct queue_entry *tail;
        size_t length;
};

struct queue *queue_create(void);
ssize_t queue_length(struct queue *q);
bool queue_is_empty(struct queue *q);
bool queue_is_full(struct queue *q);
bool queue_contains(struct queue *q, struct queue_entry *entry);
struct queue_entry *queue_entry_new(void *data);
void *queue_remove(struct queue *q, struct queue_entry *entry);
int queue_head_push(struct queue *q, void *data);
void *queue_head_pop(struct queue *q);
void *queue_head_peek(struct queue *q);
int queue_tail_push(struct queue *p, void *data);
void *queue_tail_pop(struct queue *q);
void *queue_tail_peek(struct queue *q);
int queue_flush(struct queue *q);

#endif                          /* QUEUE_H */
