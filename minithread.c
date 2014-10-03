/*
 * minithread.c:
 *      This file provides a few function headers for the procedures that
 *      you are required to implement for the minithread assignment.
 *
 *      EXCEPT WHERE NOTED YOUR IMPLEMENTATION MUST CONFORM TO THE
 *      NAMING AND TYPING OF THESE PROCEDURES.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "interrupts.h"
#include "minithread.h"
#include "queue.h"
#include "synch.h"

#include <assert.h>

#ifndef NULL
#define NULL 0
#endif // NULL

#ifndef false
#define false 0
#endif // false

#ifndef true
#define true 1
#endif // true

#ifndef quanta
#define quantum 3*SECOND
#endif

queue_t readyQueue = NULL;
queue_t deadQueue = NULL;
queue_t blockedList = NULL;

minithread_t currentThread = NULL;
minithread_t deletionThread = NULL;
semaphore_t threads_to_delete = NULL;
stack_pointer_t bogusPointer;
int lastID =0;

/*
 * A minithread should be defined either in this file or in a private
 * header file.  Minithreads have a stack pointer with to make procedure
 * calls, a stackbase which points to the bottom of the procedure
 * call stack, the ability to be enqueueed and dequeued, and any other state
 * that you feel they must have.
 */

struct minithread {
  stack_pointer_t stackbase;
  stack_pointer_t stacktop;
  proc_t body_proc;
  arg_t body_arg;
  proc_t final_proc;
  arg_t final_arg;
  int identifier;
  int isDead; // 0 ALIVE, anything else DEAD
};

/*
* This marks the currently  running thread as dead and then yields.
*/
int minithread_mark_dead(arg_t args)
{
    //printf("Thread marked as dead.\n");
    minithread_self()->isDead = true;
    semaphore_V(threads_to_delete);
    queue_append(deadQueue,minithread_self());
    minithread_yield();
    return 0; // THIS SHOULD NOT RETURN
}

/* minithread functions */

minithread_t
minithread_fork(proc_t proc, arg_t arg) {
    minithread_t newThread;
    newThread = minithread_create(proc,arg);
    queue_append(readyQueue,newThread);
    return newThread;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
    minithread_t newThread;
    //printf("Creating Thread.\n");
    newThread = (minithread_t)malloc(sizeof(struct minithread));
    minithread_allocate_stack(&(newThread->stackbase),&(newThread->stacktop));
    minithread_initialize_stack(&(newThread->stacktop),proc,arg,minithread_mark_dead,NULL);
    newThread->identifier = lastID;
    newThread->isDead = false;
    lastID++;
    return newThread;
}

minithread_t
minithread_self() {
    return currentThread;
}

int
minithread_id() {
    if(minithread_self() != NULL)
    {
        return minithread_self()->identifier;
    }
    else
    {
        return 0;
    }
}

void
minithread_stop() {
    minithread_t next = NULL;
    minithread_t temp = minithread_self();
    queue_dequeue(readyQueue,(void**)&next);
    queue_append(blockedList,minithread_self());
    if(next == NULL)
    {

        printf("Thread Stopped with no other availiable threads. Go Idle.\n\r");
        while(1);
    }
    else
    {
        currentThread = next;
        minithread_switch(&(temp->stacktop),&(currentThread->stacktop)); // SWITCH
    }

}

void
minithread_start(minithread_t t) {
    queue_delete(blockedList,t);
    queue_append(readyQueue,t);
}

void
minithread_yield() {
    minithread_t nextThread = NULL;
    minithread_t temp = NULL;
    queue_dequeue(readyQueue,(void**)&nextThread);

    if(currentThread != NULL)
    {
        if(nextThread!=NULL)
        {
            if(!(currentThread->isDead))
            {
                queue_append(readyQueue,currentThread);
            }
            temp = currentThread;
            currentThread = nextThread;
            minithread_switch(&(temp->stacktop),&(currentThread->stacktop));
        }
        else
        {
            if(!(currentThread->isDead) || currentThread == deletionThread)
            {
                printf("No availiable threads. Go Idle.\n");
                while(1){}
            }
            else
            {
                minithread_switch(&(currentThread->stacktop),&(currentThread->stacktop));
            }
        }
    }
    else
    {
        if(nextThread!=NULL)
        {
            currentThread = nextThread;
            minithread_switch(&bogusPointer,&(currentThread->stacktop));
        }
        else
        {
            printf("Error - System started without a main thread.\n");
            while(1);
        }
    }
}
/*
* A thread dedicated to deleting threads which have been marked as dead. If this is the only availiable
* thread and it yields it will not be rescheduled.
*/
int
cleanup(arg_t args) {
    minithread_t toDelete = NULL;
    while(1) {
        semaphore_P(threads_to_delete);
        queue_dequeue(deadQueue,(void**)&toDelete);
        minithread_free_stack(toDelete->stackbase);
        free(toDelete);
    }
    return 0;
};

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void 
clock_handler(void* arg)
{

}

/*
 * Initialization.
 *
 *      minithread_system_initialize:
 *       This procedure should be called from your C main procedure
 *       to turn a single threaded UNIX process into a multithreaded
 *       program.
 *
 *       Initialize any private data structures.
 *       Create the idle thread.
 *       Fork the thread which should call mainproc(mainarg)
 *       Start scheduling.
 *
 */

void
minithread_system_initialize(proc_t mainproc, arg_t mainarg) {
    readyQueue = queue_new();
    blockedList = queue_new();
    threads_to_delete = semaphore_create();
    deadQueue = queue_new();
    minithread_fork(mainproc,mainarg);
    deletionThread = minithread_fork(cleanup,NULL);
    minithread_clock_init(quanta,clock_handler);
    set_interrupt_level(ENABLED);
    minithread_yield();
}
