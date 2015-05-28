#ifndef QUEUE_H
#define QUEUE_H

/** A queue is a First-in, First-out data structure */
struct _queue {
  /** Pointer to the base of the queue, this is where the queue starts in memory. Use max_cells to calculate where it ends. */
  void **base; 
  /** Pointer to the current head of the queue, points to next availiable space*/
  void **head; 
  /** Pointer to the current tail of the queue, this is the item that would be dequeued next. Points to the actual item.*/
  void **tail; 
  /** Maximum cells this queue can hold */
  int max_cells;
  /** Current number of stored items */
  int used_cells;
};

typedef struct _queue Queue;

/** Create a queue by allocating a Queue structure in the heap and initializing it,
  *  and allocating memroy to hold the queue entries.
  * @param max_cells Maximum entires in the queue
  * @return Pointer to the newly-allocated Queue structure, NULL if error.
  */
Queue* create_queue(int max_cells);

/** Delete a queue, including the structure and memory for holding the entires.
  * Does NOT delete the entries themselves
  * @param which_queue Pointer to the Queue structure
  */
void delete_queue(Queue *which_queue);

/** Puts a pointer into the queue. This moves the head ptr and places the new item
    If there is no room on the stack, returns failure without queueing the item.
  @param which_queue Pointer to queue to be queued on
  @param item Pointer to be pushed
  @return 0 if successfull, -1 if not
  */
int enqueue(Queue *which_queue, void *item);

/** Dequeues an item - removes the pointer from the queue. This moves the tail of the queue to the next item.
    If there are no items on the stack, retursn failure without modifying the queue
  @param which_queue Pointer to queue to take from
  @return pointer to dequeued item, NULL if empty
  */
void* dequeue(Queue *which_queue);


/** Shows the item that would be dequeued, but doesn't remove it from the queue
  @param which_queue Pointer to queue to take from
  @return pointer to item, NULL if empty queue
  */
void* peek(Queue *which_queue);

/** Duplicates a queue, needs to be freed */
Queue* dupQueue(Queue *which_queue);
#endif
