/* network test program 3

     local loopback test: spawns NUM_RECEIVERS+1 threads and creates NUM_RECEIVERS ports. one
     thread acts as the sender and sends messages, one to each port,
     in a loop, with yields in between. each of the other threads is assigned a
     port, and reads messages out of it. both ports are local.

     USAGE: ./network3 <port>
*/

#include "defs.h"
#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 256
#define MAX_COUNT 100
#define NUM_RECEIVERS 20

miniport_t receive_ports[NUM_RECEIVERS];
miniport_t write_ports[NUM_RECEIVERS];

int
receive(int* arg) {
    miniport_t port = (miniport_t) arg;
    char buffer[BUFFER_SIZE];
    int length;
    int i;
    miniport_t from;

    for (i=0; i<MAX_COUNT; i++) {
        length = BUFFER_SIZE;
        minimsg_receive(port, &from, buffer, &length);
        printf("%s", buffer);
        miniport_destroy(from);
    }

    return 0;
}

int
transmit(int* arg) {
    char buffer[BUFFER_SIZE];
    int length;
    int i;
    int d;
    network_address_t my_address;

    network_get_my_address(my_address);

    for (i=0;i<NUM_RECEIVERS;i++){
        receive_ports[i] = miniport_create_unbound(i);
        write_ports[i] = miniport_create_bound(my_address, i);
    }

    for (i=0;i<NUM_RECEIVERS;i++){
        minithread_fork(receive, (int *)receive_ports[i]);
    }

    for (i=0; i<MAX_COUNT; i++) {
        for (d=0;d<NUM_RECEIVERS;d++){
            printf("Sending packet %d to receiver %d.\n", i+1,d+1);
            sprintf(buffer, "Count for receiver %d is %d.\n",d+1, i+1);
            length = strlen(buffer) + 1;
            minimsg_send(receive_ports[d], write_ports[d], buffer, length);
            minithread_yield();
        }
    }

    return 0;
}

int
main(int argc, char** argv) {
    short fromport;
    fromport = atoi(argv[1]);
    network_udp_ports(fromport,fromport); 
    minithread_system_initialize(transmit, NULL);
    return -1;
}
