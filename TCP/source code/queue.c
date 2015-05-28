/** Queue.c - implements the queue */
#include <stdlib.h>


#include "queue.h"

Queue* create_queue(int max_cells) {
  Queue *new_queue = (Queue *) malloc(sizeof(Queue));

  if (new_queue == NULL) return NULL;

  // Initialize the structure
  new_queue->max_cells = max_cells;
  new_queue->used_cells = 0;
  new_queue->base = (void**) calloc(max_cells, sizeof(void*));

  if (new_queue->base == NULL) {
    free(new_queue); // Clean up
    return NULL;
  }
  new_queue->head = new_queue->base; // Move the head to the begining of the queue
  new_queue->tail = new_queue->base; // Move the tail to the begining of the queue

  return new_queue;
}

void delete_queue(Queue *which_queue) {
  free(which_queue->base);
  free(which_queue);
}

int enqueue(Queue *which_queue, void *item) {
  // Check if queue is full
  if((which_queue->used_cells) >= (which_queue->max_cells)) {
    return -1; // Queue overflow
  }

  *(which_queue->head) = item;  // Store the pointer
  if ((which_queue->base) + (which_queue->max_cells - 1) == which_queue->head) {
    which_queue->head = which_queue->base; // If we wrap around, actually wrap around
  } else {
    (which_queue->head)++;        // If we don't wrap around, just move forward one
  }
  (which_queue->used_cells)++;

  return 0; // Success
}

void* dequeue(Queue *which_queue) {
  if ((which_queue->used_cells) <= 0) { // Underflow
    return NULL; // Underflow, return nothing
  }

  void* retval = *(which_queue->tail);
  if ((which_queue->base) + (which_queue->max_cells - 1) == which_queue->tail) {
    which_queue->tail = which_queue->base;
  } else {
    (which_queue->tail)++;
  }
  (which_queue->used_cells)--;

  return retval;
}

void* peek(Queue *which_queue) {
  if ((which_queue->used_cells) <= 0) { // Empty queue
    return NULL;
  }
  
  return *(which_queue->tail);
}

Queue* dupQueue(Queue *which_queue) {
  Queue *new_queue = (Queue *) malloc(sizeof(Queue));

  if (new_queue == NULL) return NULL;

  // Initialize the structure
  new_queue->max_cells = which_queue->max_cells;
  new_queue->used_cells = which_queue->used_cells;
  new_queue->base = (void**) calloc(which_queue->max_cells, sizeof(void*));

  if (new_queue->base == NULL) {
    free(new_queue); // Clean up
    return NULL;
  }

  // Copy the base array
  int i;
  for (i = 0; i < which_queue->max_cells; i++) {
    new_queue->base[i] = which_queue->base[i];
  }

  // Copy pointers
  new_queue->head = new_queue->base + (which_queue->head - which_queue->base);
  new_queue->tail = new_queue->base + (which_queue->tail - which_queue->base);

  return new_queue;
}
