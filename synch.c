#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

/*
 * Semaphores.
 */
struct semaphore {
  int count;
  queue_t queue;
  //int lock;
} semaphore;


/*
 * semaphore_t semaphore_create()
 *      Allocate a new semaphore.
 */
semaphore_t semaphore_create() {
  semaphore_t new   = (semaphore_t)malloc(sizeof(semaphore));
  queue_t new_q     = queue_new();

  new->count        = 0;
  new->queue        = new_q;
  //new->lock         = 0;
  return new;
}

/*
 * semaphore_destroy(semaphore_t sem);
 *      Deallocate a semaphore.
 */
void semaphore_destroy(semaphore_t sem) {
  queue_free (sem->queue);
  free (sem);
}


/*
 * semaphore_initialize(semaphore_t sem, int cnt)
 *      initialize the semaphore data structure pointed at by
 *      sem with an initial value cnt.
 */
void semaphore_initialize(semaphore_t sem, int cnt) {
  sem->count = cnt;
}

/*
 * semaphore_P(semaphore_t sem)
 *      P (decrement) on the sempahore.
 */
void semaphore_P(semaphore_t sem) {
  //while (test_and_set(&sem->lock) == 1){
  //  minithread_yield();
  //}
  if ((--(sem->count))<0){
    queue_append(sem->queue,minithread_self());
    //sem->lock = 0;
    minithread_stop(minithread_self());
  }
  //else {
  //  sem->lock = 0;
  //}
}

/*
 * semaphore_V(semaphore_t sem)
 *      V (increment) on the sempahore.
 */
void semaphore_V(semaphore_t sem) {
  void* thread = NULL;
  //while (test_and_set(&sem->lock) == 1){
  //  minithread_yield();
  //}
  if ((++(sem->count))<=0){
    queue_dequeue(sem->queue,&thread);
    minithread_start((minithread_t )thread);
  }
  //sem->lock = 0;
}
