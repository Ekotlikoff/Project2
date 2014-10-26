/*
 *  Implementation of minimsgs and miniports.
 */
#include <stdlib.h>
#include "minimsg.h"
#include "miniheader.h"
#include "synch.h"
#include "queue.h"

#define BOUND 0
#define UNBOUND 1
#ifndef NULL
#define NULL 0
#endif

typedef struct miniport
{
	int type;          		//BOUND or UNBOUND
	int port_number;
	union {
            struct {
                queue_t incoming_packets;
                semaphore_t data_ready;
            } unbound;
            
            struct {
                network_address_t remote_address;
                int remote_unbound_port;
            } bound;
	};
} miniport;

miniport_t* unbound_ports;
int* current_bound;

/* performs any required initialization o;wqq;;qwf the minimsg layer.
 */
void
minimsg_initialize()
{
	unbound_ports = (miniport_t*)calloc(32768,sizeof(miniport_t)); //Calloc because comparing unbound_ports to NULL, make sense?
}

/* Creates an unbound port for listening. Multiple requests to create the same
 * unbound port should return the same miniport reference. It is the responsibility
 * of the programmer to make sure he does not destroy unbound miniports while they
 * are still in use by other threads -- this would result in undefined behavior.
 * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
 * outside this range, it is considered an error.
 */
miniport_t
miniport_create_unbound(int port_number)
{
    miniport_t this_port;
    if (unbound_ports[port_number] == NULL) {
	this_port       		       = (miniport_t)malloc(sizeof(miniport));
	this_port->type   		       = UNBOUND;
	this_port->port_number 		       = port_number;
    	this_port->unbound.data_ready                  = semaphore_create();
    	this_port->unbound.incoming_packets            = queue_new();
    }
    else {
	this_port = unbound_ports[port_number];
    }
    return this_port;
}

/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t
miniport_create_bound(network_address_t addr, int remote_unbound_port_number)
{
    miniport_t this_port; 
    if (current_bound == NULL){
        current_bound = (int*)malloc(sizeof(int));
        *current_bound = 32768;
    }
    else if (*current_bound == 65536) { //so max val is 65535
        *current_bound = 32768;
    }
    this_port    		                 = (miniport_t)malloc(sizeof(miniport));
    this_port->type		                 = BOUND;
    this_port->port_number 	    		 = *current_bound;
    network_address_copy(addr,this_port->bound.remote_address);
    this_port->bound.remote_unbound_port               = remote_unbound_port_number;
    current_bound++;
    return this_port;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void
miniport_destroy(miniport_t miniport)
{
	if (miniport->type == UNBOUND) {
		semaphore_destroy(miniport->unbound.data_ready);
		queue_free(miniport->unbound.incoming_packets);
	}
}

/* Sends a message through a locally bound port (the bound port already has an associated
 * receiver address so it is sufficient to just supply the bound port number). In order
 * for the remote system to correctly create a bound port for replies back to the sending
 * system, it needs to know the sender's listening port (specified by local_unbound_port).
 * The msg parameter is a pointer to a data payload that the user wishes to send and does not
 * include a network header; your implementation of minimsg_send must construct the header
 * before calling network_send_pkt(). The return value of this function is the number of
 * data payload bytes sent not inclusive of the header.
 */
int
minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len)
{
    mini_header_t header = (mini_header_t)malloc(sizeof(struct mini_header));
    network_address_t temp;// = (network_address_t)malloc(2*sizeof(unsigned int));
    network_get_my_address(temp);
    //create header (miniheader.h has type def)
    header->protocol = PROTOCOL_MINIDATAGRAM;
    pack_address(header->source_address,temp);
    pack_address(header->destination_address,local_bound_port->bound.remote_address);
//    pack_unsigned_short(header->source_port,(unsigned short) //need to pack the ports
    //pack header
    //network_send_pkt()
    return 0;
}

/* Receives a message through a locally unbound port. Threads that call this function are
 * blocked until a message arrives. Upon arrival of each message, the function must create
 * a new bound port that targets the sender's address and listening port, so that use of
 * this created bound port results in replying directly back to the sender. It is the
 * responsibility of this function to strip off and parse the header before returning the
 * data payload and data length via the respective msg and len parameter. The return value
 * of this function is the number of data payload bytes received not inclusive of the header.
 */
int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len)
{
    //p on the sema
    //pop off the queue
    //unpack header unless this should be done in interrupt handler?
    //create new local bound_port
    //return port,len, and header
    //return number of data payload bytes recieved not invlusive of header
    return 0;
}

