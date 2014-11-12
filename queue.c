/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef NULL
#define NULL 0
#endif // NULL


typedef struct node{
  void* val;
  struct node* next;
} node;

struct queue{
  int length;
  node* first;
  node* last;
};

/*
 * Return an empty queue.
 */

queue_t
queue_new() {
  queue_t new = malloc (sizeof(struct queue));
  if(!new)
  {
    return NULL;
  }
  new->length = 0;
  new->first = NULL;
  new->last = NULL;
  return new;
}

void *
queue_first(queue_t queue) {
  if (!queue || queue->first == NULL){
    return NULL;
  }
  return queue->first->val;
}

node*
get(queue_t queue, int index){
  int counter;
  node* temp = queue->first;

  for (counter=0;counter<=index;counter++){
    temp = temp->next;
  }
  return temp;
}

int
queue_insert(queue_t queue, void* item, int index){
  node* previous;
  node* new; 
  if(!queue)
  {
        return -1;
  }
  if (index > queue->length){
    printf("queue_insert error - index out of bounds\n");
    return -1;
  }
  if (queue == NULL){
    return -1;
  }
  if (index == 0){
    return queue_prepend(queue, item);
  }
  else if (index == queue->length){
    return queue_append(queue, item);
  }
  new = (node *)malloc(sizeof(node));
  previous = get(queue, index-1);
  new->val = item;
  new->next = previous->next;
  previous->next = new;
  queue->length++;
  return 0;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int
queue_prepend(queue_t queue, void* item) {
  node* new = (node *)malloc(sizeof(node));
  if(!new || !queue)
  {
    return -1;
  }
  new->val = item;
  new->next = queue->first;
  queue->first = new;
  if (queue->length == 0){
    queue->last = new;
  }
  queue->length++;
  return 0;
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 * O(1)
 */

int
queue_append(queue_t queue, void* item) {
  node* new = (node *)malloc(sizeof(node));
  if(!new || !queue)
  {
    return -1;
  }
  new->val=item;
  new->next=NULL;
  if (queue->length == 0){
    queue->first= new;
    queue->last = new;
    queue->length++;
  }
  else  {
    queue->last->next = new;
    queue->last = new;
    queue->length++;
  }
  return 0;
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 * O(1)
 */
int
queue_dequeue(queue_t queue, void** item) {
  node* temp;
  if(!queue)
  {
    printf("DEQUEUE: queue is NULL\n");
    return -1;
  }
  *item = NULL;
  if (queue->length == 0){
    *item = NULL;
    printf("DEQUEUE: length is 0\n");
    return -1;
  }
  *item = queue->first->val;
  temp = queue->first;
  queue->first = temp->next;
  queue->length--;
  free(temp);
  return 0;
}

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its 2nd
 * argument and the queue element is the 1st.  Return 0 (success)
 * or -1 (failure).
 */
int
queue_iterate(queue_t queue, func_t f, void* item) {
  int counter;
  node* temp;

  if (queue == NULL || f == NULL) {
    return -1;
  }
  temp = queue->first;
  for (counter=0;counter<queue->length;counter++){
    f (temp->val,item);
    temp = temp->next;
  }
  return 0;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */

int
queue_free (queue_t queue) {
  int counter;
  node* temp1;
  node* temp2;

  if (queue == NULL){
    return -1;
  }
  temp1 = queue->first;
  for (counter=0;counter<queue->length;counter++){
    temp2 = temp1->next;
    free(temp1);
    temp1 = temp2;
  }
  free (queue);
  queue = NULL;
  return 0;
}

/*
 * Return the number of items in the queue.
 */
int
queue_length(queue_t queue) {
  if (queue == NULL) {
    return -1;
  }
  return queue->length;
}

/*
 * Delete the specified item from the given queue.
 * Return -1 on error.
 */

int
queue_delete(queue_t queue, void* item) {
  int counter;
  node* prev;
  node* current;
  node* next;

  if (queue == NULL){
      printf("QUEUE DELETE: queue is null\n");
      return -1;
  }
  if(queue->length == 0)
  {
      printf("QUEUE DELETE: queue is empty\n");
      return -1;
  }

  if(queue->first==NULL)
  {
      printf("QUEUE_DELETE: Error non-zero but first is null\n");
      return -1;
  }
  prev = NULL;
  current = queue->first;
  next = current->next;
  for(counter =0;counter < queue->length && current; counter++)
  {
      if(current->val==item)
      {
          if(prev)
          {
              prev->next = next;
          }
          else
          {
              queue->first = next;
          }
          if(!next||counter==queue->length-1)
          {
            queue->last = prev;
          }
          free(current);
          queue->length--;
          return 0;
      }
      prev = current;
      current = next;
      next = next->next;
  }
  return -1;    // if item is not in list
}
