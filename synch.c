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
  interrupt_level_t old_interrupt_level;
  old_interrupt_level = set_interrupt_level(DISABLED);
  if ((--(sem->count))<0){
    queue_append(sem->queue,minithread_self());
    minithread_stop(minithread_self());
  }
  set_interrupt_level(old_interrupt_level);
}

/*
 * semaphore_V(semaphore_t sem)
 *      V (increment) on the sempahore.
 */
void semaphore_V(semaphore_t sem) {
  interrupt_level_t old_interrupt_level;
  void* thread = NULL;
  old_interrupt_level = set_interrupt_level(DISABLED);
  if ((++(sem->count))<=0){
    queue_dequeue(sem->queue,&thread);
    minithread_start((minithread_t )thread);
  }
  set_interrupt_level(old_interrupt_level);
}
