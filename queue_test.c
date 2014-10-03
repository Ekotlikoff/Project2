#include "queue.h"
#include "queue.c"
#include <stdio.h>
#include <string.h>

int test(queue_t queue,int* listy,int len){
  int counter;
  node* current = queue->first;

  if (queue->length != len){
    printf("queue length, %i != listy length, %i\n",queue->length,len);
    return -1;
    }

  for(counter=0;counter<queue->length;counter++){
    if (*(int *)current->val != listy[counter]){
      printf("index %i does not match",counter);
      return -1;
      }
    current = current->next;
    }
  return 0;
  }

void iter_tester(void* elem,void* running_tot){
  printf("elem: %i, tunning total: %i\n",*(int *)elem, *(int *)running_tot);
  *(int *)running_tot += *(int *) elem;
  }

int main(){
  int counter;
  int arraya[5] = {1,2,3,4,5};
  int arrayb[] = {3,4,5};
  int arrayc[] = {2,4,5};
  int* inty = malloc(sizeof(int *));
  int** item1 = malloc(sizeof(int **));
  int** item2 = malloc(sizeof(int **));
  int iny = 3;

  queue_t queue = queue_new();
  printf("prepend prior\n");
  queue_prepend(queue, &iny);
  printf("prepend post\n");
  int inty1 = 2;
  queue_prepend(queue, &inty1);
  printf("prepend post\n");
  int inty2 = 1;
  queue_prepend(queue, &inty2);
  int inty3 = 4;
  queue_append(queue, &inty3);
  printf("prepend post\n");
  int inty4 = inty3+1;
  queue_append(queue, &inty4);
  if (test(queue,arraya,5) == 0){
    printf("test 1 PASSED\n");
    }
  else {
    printf("test 1 FAILED\n");
    }
  queue_dequeue(queue,(void **)item1);  
  queue_dequeue(queue,(void **)item2);
  if (test(queue,arrayb,3) == 0 && **item1 == 1 && **item2 == 2){
    printf("test 2 PASSED\n");
    }
  else {
    printf("test 2 FAILED\n");
    }
  queue_prepend(queue, &inty1);
  *inty = 0;
  queue_iterate(queue,iter_tester,(void *)inty);
  if (*inty == 14){
    printf("test 3 PASSED\n");
    }
  else{
    printf("test 3 FAILED, total = %i\n",*inty);
    }
  queue_delete(queue, &iny);
  if (test(queue,arrayc,3) == 0){
    printf("test 4 PASSED\n");
    }
  else {
    printf("test 4 FAILED\n");
    }
  queue_t q = queue_new();
  for(counter=0;counter<100;counter++){
    printf("Index %i\n",counter);
    queue_append(q,&counter);
    }
  if (q->length = 100){
    printf("test 5 PASSED\n");
    }
  else {
    printf("test 5 FAILED\n");
    }
  queue_free(q);
  queue_free(queue);
  free(inty);
  free(item1);
  free(item2);
  }
