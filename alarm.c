#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"


typedef struct alarm alarm;
struct alarm{
	long when_to_execute;
	alarm_handler_t alarm;
	void *arg;
};

typedef struct tuple tuple;
struct tuple{
	int *fst;
	int *snd;
};

alarm_id alarm_queue;

void
ring_alarm(){
    alarm* this_alarm = NULL;
  	if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}   
    if (-1 == queue_dequeue(alarm_queue,(void *)this_alarm)){
        printf("ring alarm error\n");        
        return;
    }
    this_alarm->alarm(this_alarm->arg);
}

int
first_execution_tick(){    //returns -1 if no alarms
    alarm* this_alarm = NULL;
    if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}   
    this_alarm = queue_first(alarm_queue);
    if (this_alarm == NULL){
        return -1;
    }
    return this_alarm->when_to_execute;
}

/* func used in iterate to find place to insert alarm */
void
func (void* element, void* arg){             //arg is tuple, fst is the to be inserted alarm's when_to_execute
	alarm* this_alarm = (alarm *) element;
	tuple* out        = (tuple *) arg;
	if (*out->fst <= this_alarm->when_to_execute){//TODO when ticks rap around this will get slow temporarily
		return;
	}
	*out->snd += 1;
}

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm_f, void *arg)
{
	float millis_per_tick;
	int ticks_to_wait;
	void* item = (void *)malloc(sizeof(int));
	alarm* new = (alarm *)malloc(sizeof(alarm));
	if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}
	millis_per_tick = (float)clock_quantum/(float)MILLISECOND;
	ticks_to_wait = ceil((float) delay / millis_per_tick);
	new->when_to_execute = get_clock_ticks() + ticks_to_wait;
	new->alarm = alarm_f;
	new->arg = arg;
	queue_iterate(alarm_queue,func,item);
	queue_insert(alarm_queue,(void *)new,*(int*)item);
    return new;
}

/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
    if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}
    if (queue_delete(alarm_queue, alarm) == -1){  // nothing deleted
		return 1;   
    }
    return 0;
}

/*
** vim: ts=4 sw=4 et cindent
*/
