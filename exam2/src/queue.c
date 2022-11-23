/*
 * Code courtesy of Kristjon from plenaries:
 * https://github.com/kr1stj0n/plenaries-in3230-in4230-h22/tree/main/p7_28-10-2022/libqueue
 *
 * Made some changes. The original wasn't ideal for my use. The only way to
 * iterate over the queue was to free and allocate for each entry, even if no
 * changes are made.
 *
 * Changes:
 * - Removed `id` field. It seemingly served no function?
 * - Linux kernel style indentation.
 * - Added `queue_remove()` which selectively removes an entry.
 * - Modified the "pop" functions to use `queue_remove()`.
 * - TODO: There should be an interface to "skim" the entries, without freeing
 *   them along the way (I just do it manually now).
 */

#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

struct queue *queue_create(void)
{
        struct queue *q = (struct queue *)calloc(1, sizeof(struct queue));
        if (!q)
                return NULL;

        q->head = NULL;
        q->tail = NULL;
        q->length = 0;

        return q;
}

ssize_t queue_length(struct queue *q)
{
        if (!q)
                return -1;

        return q->length;
}

struct queue_entry *queue_entry_new(void *_data)
{
        struct queue_entry *entry = (struct queue_entry *)calloc(1,
                                                                 sizeof(struct
                                                                        queue_entry));
        if (!entry)
                return NULL;

        entry->data = _data;
        entry->next = NULL;
        entry->prev = NULL;

        return entry;
}

bool queue_is_empty(struct queue *q)
{
        if (!q) {
                printf("[ERROR]: Cannot check the emptiness of a NULL queue");
                return true;
        }

        return q->length == 0;
}

bool queue_is_full(struct queue *q)
{
        if (!q) {
                printf("[ERROR]: Can't check the fullness of a NULL queue");
                return true;
        }

        return (queue_length(q) == MAX_QUEUE_SIZE);
}

int queue_head_push(struct queue *q, void *data)
{
        struct queue_entry *entry;

        if (!q) {
                printf("[ERROR]: Cannot head-push on a NULL queue\n");
                return -1;
        }

        if (queue_is_full(q)) {
                printf
                    ("[WARNING]: queue is NULL OR cannot head-push on a FULL queue\n");
                return -1;
        }

        entry = queue_entry_new(data);
        if (!entry)
                return -1;

        /* Insert into the queue */
        if (!q->head) {
                /* If the queue was previously empty, both the head and tail
                   must be pointed at the new entry */
                q->head = entry;
                q->tail = entry;
        } else {
                /* First entry in the list must have prev pointed back to this
                   new entry */

                q->head->prev = entry;

                /* Only the head must be pointed at the new entry */

                q->head = entry;
        }

        q->length++;

        /* printf("[DEBUG]: Entry %p head-pushed into queue %p (length = %zd)\n", */
        /*        (void *)entry, (void *)q, q->length); */

        return 0;
}

bool queue_contains(struct queue *q, struct queue_entry *entry)
{
        for (struct queue_entry *e = q->head; e != NULL; e = e->next)
                if (e == entry)
                        return true;
        return false;
}

void *queue_remove(struct queue *q, struct queue_entry *entry)
{
        void *data;

        if (!q) {
                printf("Cannot remove from a NULL queue");
                return NULL;
        }

        if (!entry) {
                printf("Cannot remove from a NULL entry from queue");
                return NULL;
        }

        if (queue_is_empty(q)) {
                printf("[ERROR]: queue %p is empty, can't remove\n", (void *)q);
                return NULL;
        }

        if (!queue_contains(q, entry)) {
                printf("Entry not in queue");
                return NULL;
        }

        if (q->head == entry) {
                /* Unlink the first entry from the head of the queue */
                q->head = entry->next;
                data = entry->data;

                if (!q->head)
                        /* If doing this has unlinked the last entry in the
                         * queue, set tail to NULL as well. */
                        q->tail = NULL;
                else
                        /* The new first in the queue has no previous entry */
                        q->head->prev = NULL;

        } else if (q->tail == entry) {
                /* Unlink the first entry from the tail of the queue */
                q->tail = entry->prev;
                data = entry->data;

                if (!q->tail)
                        /* If doing this has unlinked the last entry in the
                         * queue, set head to NULL as well. */
                        q->head = NULL;
                else
                        /* The new entry at the tail has no next entry. */
                        q->tail->next = NULL;
        } else {
                /* If removed entry is neither head of tail, fix pointers in
                 * adjacent entries. */
                entry->prev->next = entry->next;
                entry->next->prev = entry->prev;
        }

        /* printf("[DEBUG]: Entry %p removed from queue %p (length = %zd)\n", */
        /*        (void *)entry, (void *)q, q->length); */

        free(entry);
        q->length--;

        return data;
}

void *queue_head_pop(struct queue *q)
{
        void *data;

        /* printf("[DEBUG]: Entry %p head-popped from queue %p (length = %zd)\n", */
        /*        (void *)q->head, (void *)q, q->length); */

        data = queue_remove(q, q->head);

        return data;
}

int queue_tail_push(struct queue *q, void *data)
{
        struct queue_entry *entry;

        if (!q) {
                printf("Cannot tail-push on a NULL queue");
                return -1;
        }

        if (queue_is_full(q) == 1) {
                printf
                    ("[WARNING]: queue is NULL | Cannot head-push on a FULL queue\n");
                return -1;
        }

        entry = queue_entry_new(data);
        if (!entry)
                return -1;

        /* Insert into the queue tail */

        if (!q->tail) {

                /* If the queue was previously empty, both the head and tail
                   must be pointed at the new entry */

                q->head = entry;
                q->tail = entry;
        } else {

                /* The current entry at the tail must have next pointed to this
                   new entry */

                q->tail->next = entry;

                /* Only the tail must be pointed at the new entry */

                q->tail = entry;
        }

        q->length++;

        /* printf("[DEBUG]: Entry %p tail-pushed into queue %p (length = %zd)\n", */
        /*        (void *)entry, (void *)q, q->length); */

        return 0;
}

void *queue_tail_pop(struct queue *q)
{
        void *data;

        data = queue_remove(q, q->tail);

        /* printf("[DEBUG]: Entry %p tail-popped from queue %p (length = %zd)\n", */
        /*        (void *)q->tail, (void *)q, q->length); */

        return data;
}

void *queue_head_peek(struct queue *q)
{
        void *data;

        if (!q) {
                printf("Cannot head-pop from a NULL queue");
                return NULL;
        }

        if (queue_is_empty(q)) {
                printf("[ERROR]: queue %p is empty, can't head-pop\n", (void *)q);
                return NULL;
        }

        data = q->head->data;

        /* printf("[DEBUG]: Entry %p head-peeked from queue %p (length = %zd)\n", */
        /*        (void *)q->head, (void *)q, q->length); */

        return data;
}

void *queue_tail_peek(struct queue *q)
{
        void *data;

        if (!q) {
                printf("Cannot tail-pop from a NULL queue");
                return NULL;
        }

        if (queue_is_empty(q)) {
                printf("[ERROR]: queue %p is empty, can't tail-pop\n", (void *)q);
                return NULL;
        }

        data = q->tail->data;

        /* printf("[DEBUG]: Entry %p tail-peeked from queue %p (length = %zd)\n", */
        /*        (void *)q->tail, (void *)q, q->length); */

        return data;
}

int queue_flush(struct queue *q)
{
        /* Empty the queue */
        while (!queue_is_empty(q))
                queue_head_pop(q);

        /* Free back the queue */
        free(q);

        /* printf("[DEBUG]: Queue destroyed successfully!\n"); */

        return 0;
}
