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
#include "multilevel_queue.h"
#include "synch.h"
#include "alarm.h"
#include "network.h"
#include "miniheader.h"
#include "minimsg.h"

#include <assert.h>

#include <limits.h>

#ifndef NULL
#define NULL 0
#endif // NULL

#ifndef false
#define false 0
#endif // false

#ifndef true
#define true 1
#endif // true

#ifndef clock_quantum
#define clock_quantum (SECOND/10)
#endif

long *clock_ticks;

multilevel_queue_t readyQueue = NULL;
queue_t deadQueue = NULL;
queue_t blockedList = NULL;

minithread_t currentThread = NULL;
minithread_t deletionThread = NULL;
semaphore_t threads_to_delete = NULL;
stack_pointer_t bogusPointer;

int lastID = 0;

int starveCounter = 0;
int yieldFlag = 0; // 1 if the last thread yielded 0 otherwise

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
  int ticksToDeschedule;
  int queueLevel;
};

minithread_t getNextThread()
{
    minithread_t next;
    //FIRST GO 0 to 3
    if(starveCounter%2==0) //get level 0 50% of the time
    {
        multilevel_queue_dequeue(readyQueue,0,(void**)&next);
    }
    else if( (starveCounter-1)%4 == 0) // get level 1 25%
    {
        multilevel_queue_dequeue(readyQueue,1,(void**)&next);
    }
    else if(starveCounter ==3 || starveCounter == 11 || starveCounter == 19) // level 2 15%
    {
        multilevel_queue_dequeue(readyQueue,2,(void**)&next);
    }
    else if(starveCounter ==7 || starveCounter == 15 ) //level 3 10%
    {
        multilevel_queue_dequeue(readyQueue,3,(void**)&next);
    }
    else
    {
        printf("ERROR ON NEXT THREAD THIS SHOULD NEVER OCCUR \n");
        return NULL;
    }
    starveCounter++;
    starveCounter = starveCounter%20;
    return next;
}
/*
* This marks the currently  running thread as dead and then yields.
*/
int minithread_mark_dead(arg_t args)
{
    interrupt_level_t last;
    last = set_interrupt_level(DISABLED);
    //printf("Thread marked as dead.\n");
    minithread_self()->isDead = true;
    semaphore_V(threads_to_delete);
    queue_append(deadQueue,minithread_self());
    set_interrupt_level(last);
    set_interrupt_level(last);
    minithread_yield();
    return 0; // THIS SHOULD NOT RETURN
}

/* minithread functions */

minithread_t
minithread_fork(proc_t proc, arg_t arg) {
    minithread_t newThread;
    interrupt_level_t last = set_interrupt_level(DISABLED);
    newThread = minithread_create(proc,arg);
    multilevel_queue_enqueue(readyQueue,0,newThread);
    set_interrupt_level(last);
    return newThread;
}

minithread_t
minithread_create(proc_t proc, arg_t arg) {
    minithread_t newThread;
    interrupt_level_t last;
    last = set_interrupt_level(DISABLED);
    newThread = (minithread_t)malloc(sizeof(struct minithread));
    if(!newThread)
    {
        set_interrupt_level(last);
        return NULL;
    }
    minithread_allocate_stack(&(newThread->stackbase),&(newThread->stacktop));
    minithread_initialize_stack(&(newThread->stacktop),proc,arg,minithread_mark_dead,NULL);
    newThread->identifier = lastID;
    newThread->isDead = false;
    newThread->ticksToDeschedule = 1;
    newThread->queueLevel = 0;
    lastID++;
    set_interrupt_level(last);
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
    interrupt_level_t last = set_interrupt_level(DISABLED);
    minithread_t temp = minithread_self();
    next = getNextThread();
    queue_append(blockedList,minithread_self());
    if(next == NULL)
    {

        printf("Thread Stopped with no other availiable threads. Go Idle.\n\r");
        set_interrupt_level(ENABLED);
        while(1);
    }
    else
    {
        currentThread = next;
        set_interrupt_level(last);
        minithread_switch(&(temp->stacktop),&(currentThread->stacktop)); // SWITCH
    }

}

void
minithread_start(minithread_t t) {

    interrupt_level_t last = set_interrupt_level(DISABLED);
    queue_delete(blockedList,t);
    multilevel_queue_enqueue(readyQueue,t->queueLevel,t);
    set_interrupt_level(last);
}

void
minithread_yield() {
    interrupt_level_t last;
    minithread_t nextThread = NULL;
    minithread_t temp = NULL;
    nextThread = getNextThread();
    yieldFlag = 1;
    last = set_interrupt_level(DISABLED);
    if(currentThread != NULL)
    {
        if(nextThread!=NULL)
        {
            if(!(currentThread->isDead))
            {
                multilevel_queue_enqueue(readyQueue,currentThread->queueLevel,currentThread);
            }
            temp = currentThread;
            currentThread = nextThread;
            set_interrupt_level(last);
            minithread_switch(&(temp->stacktop),&(currentThread->stacktop));
        }
        else
        {
            if(!(currentThread->isDead) || currentThread == deletionThread)
            {
                printf("No availiable threads. Go Idle.\n");
                set_interrupt_level(ENABLED);
                while(1){}
            }
            else
            {
                set_interrupt_level(last);
                minithread_switch(&(currentThread->stacktop),&(currentThread->stacktop));
            }
        }
    }
    else
    {
        if(nextThread!=NULL)
        {
            currentThread = nextThread;
            set_interrupt_level(last);
            minithread_switch(&bogusPointer,&(currentThread->stacktop));
        }
        else
        {
            printf("Error - System started without a main thread.\n");
            set_interrupt_level(ENABLED);
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

long get_clock_ticks(){
    return *clock_ticks;
}

int get_quantum(){
    return clock_quantum;
}

void increment_clock_ticks(long *clock_ticks){
    if (*clock_ticks == LONG_MAX){
        *clock_ticks = 0;
        return;
    }
    //printf("Clock ticks: %ld \n",*clock_ticks);
    *clock_ticks+=1;
}

/*
 * This is the clock interrupt handling routine.
 * You have to call minithread_clock_init with this
 * function as parameter in minithread_system_initialize
 */
void
clock_handler(void* arg)
{
    minithread_t nextThread = NULL;
    minithread_t temp = NULL;
    interrupt_level_t old_interrupt_level;

    old_interrupt_level = set_interrupt_level(DISABLED);
    while (get_clock_ticks() == first_execution_tick()){
        //printf("About to ring_alarm\n");
        ring_alarm();
    }
    increment_clock_ticks(clock_ticks);
    if(currentThread)
    {
        if(yieldFlag == 0 )
        {
            currentThread->ticksToDeschedule--;
        }
        else
        {
            yieldFlag = 0;
        }
        if(currentThread->ticksToDeschedule == 0)
        {
            currentThread->queueLevel++;
            if(currentThread->queueLevel>3)
            {
                currentThread->queueLevel=3;
            }
            currentThread->ticksToDeschedule = 1<< currentThread->queueLevel;
            nextThread = getNextThread();
            if(nextThread != NULL)
            {
                //printf("SWAP %i for % i\n",currentThread->identifier,nextThread->identifier);
                multilevel_queue_enqueue(readyQueue,currentThread->queueLevel,currentThread);
                temp = currentThread;
                currentThread = nextThread;
                minithread_switch(&(temp->stacktop),&(currentThread->stacktop));
            }
            else
            {
                //printf("RETURN TO %i\n",minithread_id(currentThread));
                minithread_switch(&(currentThread->stacktop),&(currentThread->stacktop));
            }
        }
        else
        {
            //printf("Continue %i. %i ticks remain.\n",minithread_id(currentThread),currentThread->ticksToDeschedule);
            minithread_switch(&(currentThread->stacktop),&(currentThread->stacktop));
        }
    }
    else
    {
        nextThread = getNextThread();
        if(nextThread != NULL)
        {
            //printf("FIRST SCHEDULE\n");
            currentThread = nextThread;
            minithread_switch(&bogusPointer,&(currentThread->stacktop));
        }
        else
        {
            printf("This shouldn't happen\n");
            //both are null THIS SHOULD NOT HAPPEN
        }
    }
    set_interrupt_level(old_interrupt_level);
}

void
f(void* arg){  // helper function for minithread_sleep_with_timeout
  semaphore_V((semaphore_t) arg);
}

void
minithread_sleep_with_timeout(int delay){
    semaphore_t signal = semaphore_create();
    register_alarm(delay,f,(void *)signal);
    semaphore_P(signal);
    semaphore_destroy(signal);
}

void
network_handler(network_interrupt_arg_t* packet){
    miniport_t this_port;
    mini_header_t header;
    interrupt_level_t last = set_interrupt_level(DISABLED);
    int port_num;
    header = (mini_header_t)packet->buffer;
    port_num = (int)unpack_unsigned_short(header->destination_port);
    // check if PROTOCOL_MINIDATAGRAM or PROTOCOL_MINISTREAM
    // if PROTOCOL_MINISTREAM
    // check if socket_exists, if no return;
    // check if packet is control flow or contains data
    // if control -> call socket's function
    // if data    -> call socket's handle data
    // if PROTOCOL_MINIDATAGRAM
    if(header->protocol==PROTOCOL_MINIDATAGRAM) {
        if (port_exists(port_num) == 0){ //if port has not been created by user drop AND FREE the packet
            if(packet) {
                free(packet);
            }
            set_interrupt_level(last);
            return;
        }
        else { //otherwise store it in the deisgnated port's queue to be received and free it later
            this_port = miniport_create_unbound(port_num);
            queue_append(port_get_queue(this_port),(void*)packet);
            semaphore_V(port_get_sema(this_port));
            set_interrupt_level(last);
            return;
       }
    }
    else if(header->protocol==PROTOCOL_MINISTREAM) {
        return;
    }
    else {
        return;
    }
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
    readyQueue = multilevel_queue_new(4);
    blockedList = queue_new();
    threads_to_delete = semaphore_create();
    deadQueue = queue_new();
    minithread_fork(mainproc,mainarg);
    deletionThread = minithread_fork(cleanup,NULL);
    minithread_clock_init(clock_quantum,clock_handler);
    network_initialize(network_handler); // TODO is this the right place for this?
    clock_ticks = (long *)malloc(sizeof(long));
    *clock_ticks = 0;
    set_interrupt_level(ENABLED);
    minithread_yield();
}
