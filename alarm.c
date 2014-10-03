#include <stdlib.h>
#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

typedef struct alarm alarm;
struct alarm{
	int id;
	long when_to_execute;
	alarm_handler_t alarm;
	void *arg;
};

alarm_id alarm_queue;

int running_id = 0;



/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm_f, void *arg)
{
	alarm* new = (alarm *)malloc(sizeof(alarm));
	if (alarm_queue == NULL){
		alarm_queue = (alarm_id)queue_new();
	}
	new->id = running_id;
	running_id++;
	new->when_to_execute = get_clock_ticks() + delay;
	new->alarm = alarm_f;
	new->arg = arg;

    return new;
}

/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
    return 1;
}

/*
** vim: ts=4 sw=4 et cindent
*/
