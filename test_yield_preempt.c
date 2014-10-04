/* test1.c

   Spawn a single thread.
*/

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>



int
thread1(int* arg) {
    int count = 0;
    printf("T1\n");
    while(1){
        count++;
        if(count >= 50000000) {
            printf("T1\n");
            count = 0;
        }
    }
    return 0;
}
int
thread2(int* arg) {
    int count = 0;
    printf("T2\n");
    while(1){
        count++;
        if(count >= 50000000) {
            printf("T2\n");
            count = 0;
        }
    }
    return 0;
}
int
thread3(int* arg) {
    while(1){
        printf("T3\n");
        minithread_yield();
    }
    return 0;
}
int
thread0(int* arg) {
    int count = 0;
    printf("T0\n");
    minithread_fork(thread1,NULL);
    minithread_fork(thread2,NULL);
    minithread_fork(thread3,NULL);
    while(1){
        count++;
        if(count >= 50000000) {
            printf("T0\n");
            count = 0;
        }
    }
    return 0;
}
int
main(void) {
  minithread_system_initialize(thread0, NULL);
  return 0;
}
