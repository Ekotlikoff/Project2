#include <stdlib.h>
#include <stdio.h>

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
    alarm* this_alarm;
  	if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}   
    if (-1 == queue_dequeue(alarm_queue,(void **)&this_alarm)){
        printf("ring alarm error\n");        
        return;
    }
    printf("Ringing alarm\n");
    this_alarm->alarm(this_alarm->arg);
    printf("rung\n");
    free(this_alarm);
}

int
first_execution_tick(){    //returns -1 if no alarms
    alarm* this_alarm = NULL;
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

int
ceiling(double x){
    if (x - (int)x > 0){
        return (int) x + 1;
    }
    return (int) x;
}

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm_f, void *arg)
{
    double millis_per_tick;
	int ticks_to_wait;
	tuple* item;
    alarm* new;
    interrupt_level_t old_interrupt_level;
    old_interrupt_level = set_interrupt_level(DISABLED);
    if (alarm_queue == NULL){
        alarm_queue = (alarm_id)queue_new();
    }
    item = (tuple *)malloc(sizeof(tuple));
    item->snd = malloc(sizeof(int));
    *item->snd = 0;
	new = (alarm *)malloc(sizeof(alarm));
	if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}
	millis_per_tick = (double)get_quantum()/(double)MILLISECOND;
	ticks_to_wait = ceiling((double)delay / millis_per_tick);
	new->when_to_execute = get_clock_ticks() + ticks_to_wait;
	new->alarm = alarm_f;
	new->arg = arg;
    item->fst = (int*)&(new->when_to_execute);
	queue_iterate(alarm_queue,func,(void*)item);
    if (queue_length(alarm_queue) == 0){
        printf("length is 0 and appending\n");
        queue_append(alarm_queue,(void*)new);
    }
    else{
        printf("inserting at index = %i\n",*(int*)item->snd);
	    queue_insert(alarm_queue,(void *)new,*(int*)item->snd);
    }
    free(item->snd);
    free(item);
	set_interrupt_level(old_interrupt_level);
    return new;
}

/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
	interrupt_level_t old_interrupt_level;
    old_interrupt_level = set_interrupt_level(DISABLED);
    if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}
    if (queue_delete(alarm_queue, alarm) == -1){  // nothing deleted
    	set_interrupt_level(old_interrupt_level);
		return 1;   
    }
    free(alarm);
    set_interrupt_level(old_interrupt_level);
    return 0;
}

/*
** vim: ts=4 sw=4 et cindent
*/
