#include <stdlib.h>
#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

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

/* func used in iterate to find place to insert alarm */
void
func (void* element, void* arg){             //arg is tuple, fst is the to be inserted alarm's when_to_execute
	alarm* this_alarm = (alarm *) element;
	(tuple *) out     = (tuple *) out;
	if (out->fst <= element->when_to_execute){//TODO when ticks rap around this will get slow temporarily
		return;
	}
	*out->snd += 1;
}

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm_f, void *arg)
{
	void* item = (void *)malloc(sizeof(int));
	alarm* new = (alarm *)malloc(sizeof(alarm));
	if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}
	new->when_to_execute = get_clock_ticks() + delay;
	new->alarm = alarm_f;
	new->arg = arg;
	queue_iterate(alarm_queue,func,item);
	queue_insert(alarm_queue,(void *)new,(int)*item);
    return new;
}

/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
	if (queue_delete(alarm_queue, alarm) == -1){  // nothing deleted
		return 1;   
    }
    return 0;
}

/*
** vim: ts=4 sw=4 et cindent
*/
