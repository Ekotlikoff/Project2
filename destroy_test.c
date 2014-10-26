#include "defs.h"
#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>

#define BUFFER_SIZE 256
#define NUM_MSGS 10

int main(int argc, char** argv){
    int i;
    char buffer[256];
    network_address_t my_address;
    miniport_t unbound = miniport_create_unbound(0);
    miniport_t bound   = miniport_create_bound(my_address,0);
    network_get_my_address(my_address);
    for (i=0;i<NUM_MSGS;i++){
        minimsg_send(unbound,bound,buffer,strlen(buffer)+1);
    }
    miniport_destroy(unbound);
    miniport_destroy(bound);
    return 0;
}
