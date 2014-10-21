/*
 * Multilevel queue manipulation functions
 */
#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

struct multilevel_queue {
	int number_of_levels;
	int tot_length;         // number of items stored at all levels
	queue_t* levels;
};

/*
 * Returns an empty multilevel queue with number_of_levels levels. On error should return NULL.
 */
multilevel_queue_t multilevel_queue_new(int number_of_levels)
{
    int counter = 0;        // for-loop counter
    multilevel_queue_t new;
    if(number_of_levels == 0)
    {
        return NULL;
    }

	new = (multilevel_queue_t)malloc(sizeof(struct multilevel_queue));
	new->number_of_levels  = number_of_levels;
	new->tot_length        = 0;
	new->levels  	       = (queue_t*)malloc(new->number_of_levels * sizeof(queue_t));
	for (;counter<new->number_of_levels;counter++){
		new->levels[counter] = queue_new();
	}
	return new;
}

int level_out_of_range(multilevel_queue_t queue, int level){
	if (queue->number_of_levels <= level){
		//printf("Level out of range, dequeue failed\n");
		return 1;
	}
	return 0;
}

int increment_level(multilevel_queue_t queue, int level){
	level++;
	if (level_out_of_range(queue,level)){
		return 0;
	}
	return level;
}

/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item)
{
	if (level_out_of_range(queue, level)){
		return -1;
	}
	queue->tot_length++;
	return queue_append(queue->levels[level], item);
}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level.
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item)
{
	int counter = 0;        // for-loop counter

	if (queue->tot_length == 0){
		*item = NULL;
		return -1;
	}
	if (level_out_of_range(queue, level)){
		return -1;
	}
	for (counter = 0;counter<queue->number_of_levels;counter++){
		if (queue_length(queue->levels[level]) > 0) {
			if (queue_dequeue(queue->levels[level], item) == -1){
				//printf("Dequeue failure\n");
				return -1;
			}
			queue->tot_length--;
			return level;
		}
		level = increment_level(queue,level);    //TODO will this change level in the next loop?
	}
	//printf("Queue was empty when it shouldn't have been\n");
	return  -1;
}

/*
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue)
{
	int counter = 0;        // for-loop counter
	for (;counter<queue->number_of_levels;counter++){
		if (queue_free(queue->levels[counter]) == -1){
			return -1;
		}
	}
	free(queue->levels);
	free(queue);
	return 0;
}
