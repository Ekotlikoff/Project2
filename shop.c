#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "minithread.h"

//Retail Store
//
//num_employees threads unpacking phones, num_customers recieving phones
//
//TODO phones don't really 'have' serial numbers

int num_employees;
int num_customers;
int phone_count = 0;
semaphore_t phones;
semaphore_t customers;

// An employee unpacks a phone
int
unpack_phone(int* arg){
  while (1){
    semaphore_V(phones);
    printf("Unpacked\n");
    minithread_yield();
  }
}

int
receive_phone(int* arg){
  printf("waiting\n");
  semaphore_P(phones);    // get a phone
  printf("I received phone %i!\n",phone_count++);
  semaphore_P(customers); // I've been served
  return 0;
}

int main1(int* arg){
  int counter;
  for (counter=0;counter<num_employees;counter++) {
    minithread_fork(unpack_phone,&counter);
  }
  for (counter=0;counter<num_customers;counter++) {
    minithread_fork(receive_phone,&counter);
  }
  printf("%i employees busy unpacking\n",num_employees);
  printf("%i customers eager for their phones!\n",num_customers);
  return 0;
}

int
main(int argc, char **argv){
  customers = semaphore_create();
  phones    = semaphore_create();

  if (argc < 3){
    num_employees = 5;
    num_customers = 20;
  }
  else{
    num_employees = atoi (argv[1]);
    num_customers = atoi (argv[2]);
  }
  minithread_system_initialize (main1,NULL);
  return 0;
}
