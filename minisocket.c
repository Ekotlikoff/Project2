/*
 *	Implementation of minisockets.
 */
#include <stdlib.h>
#include <stdio.h>
#include "minisocket.h"
#include "miniheader.h"
#include "synch.h"
#include "alarm.h"
#include "queue.h"


#define client_upperbound 65536
#define server_upperbound 32768
#ifndef NULL
#define NULL 0
#endif

struct minisocket
{
	int               port_number;
	network_address_t remote_address;
	int 			  remote_port;
	int 			  initialized; // = 0 (set to 1 if ack received after SYNACK (SERVER) and 1 if ack sent(client)) (0 if dying)
	int 			  socket_busy; // = 0 (set to 1 if client tried to connect to already paired server)
	int 			  send_ack_received;
	semaphore_t 	  server_waiting; //sema(0) for server waiting for connection
	semaphore_t 	  receive_sema;//(0) for queue (for receive calls to P on and data_handle to V on) and
	semaphore_t 	  outer_receieve_sema; //outer sema(1)
	queue_t 		  packet_queue;
	int 			  seq_number;
	int 			  ack_number;
	semaphore_t  	  send_sema; //(0) used for retransmissions in send and init
	semaphore_t 	  outer_send_sema; //(1) to make sure only one person is manipulating send_sema at a time
	void (*handle)(minisocket_t, mini_header_reliable_t);//handle 			  handle_function; //pointer to current control flow handling function
};


void set_handle (minisocket_t, int);
// big array of clients and servers
minisocket_t ports[client_upperbound];
//current client port number
int* current_client;

// returns current control flow function, see below for all the options
void (*get_handle_function(minisocket_t socket))(minisocket_t, mini_header_reliable_t){
	return socket->handle;
}

// CONTROL FLOW FUNCTIONS
// SERVER
void handle_SYN (minisocket_t socket, mini_header_reliable_t header){ //1
	//MSG_SYN -> set_handle(socket, 2)
	//			 if packet's p_seq == this ack + 1 set this socket's ack to p_seq and continue, else return?
	//			 V(server_waiting)
	//			 set remote_address
	//			 set remote_port
	//			 server will wake up and send SYNACK with retransmitions
	//else drop
	if(header->message_type == MSG_SYN && *header->seq_number == socket->ack_number+1)  {
			socket->ack_number++;
            unpack_address((header->source_address),socket->remote_address);
            socket->remote_port = unpack_unsigned_short(header->source_port);
            semaphore_V(socket->server_waiting);
            set_handle(socket,2);
	}
}
// SERVER AND CLIENT ARE NOW PAIRED
void handle_control_server (minisocket_t socket, mini_header_reliable_t header){ //2
    network_address_t this_network_address;
    struct mini_header_reliable resp; //mini_header_reliable_t response = (mini_header_reliable_t)malloc(sizeof(mini_header_reliable));
    unsigned short int this_port;
    network_address_t myaddress;
    mini_header_reliable_t response = &resp;
    network_get_my_address(myaddress);
    unpack_address(header->source_address,this_network_address);
    this_port = unpack_unsigned_short(header->source_port);
	if (socket->initialized == 0){
		// if packet is MSG_SYN, if it's from different addr,port then remote_address remote_port reply with MSG_FIN
		// else if is MSG_ACK with ack == seq ack notify the socket by setting initialize flag to 1
		if(header->message_type==MSG_SYN &&
            (this_port!=socket->remote_port || network_compare_network_addresses(this_network_address,socket->remote_address)==0)) {
                response->protocol = PROTOCOL_MINISTREAM;
                pack_address(response->destination_address,this_network_address);
                pack_address(response->source_address,myaddress);
                pack_unsigned_short(response->destination_port,this_port);
                pack_unsigned_short(response->source_port,socket->port_number);
                response->message_type = MSG_FIN;
                pack_unsigned_int(response->seq_number, socket->seq_number);
                pack_unsigned_int(response->ack_number, socket->ack_number);// I DON'T KNOW WHAT VALUE TO PLACE
                network_send_pkt((unsigned int*)response->destination_address,sizeof(struct mini_header_reliable),(char*)header,0,NULL);
                return;
		}
        else if(header->message_type == MSG_ACK && *header->ack_number == socket->seq_number) {
        	socket->initialized = 1;
		}
            }
	else {
		//if MSG_SYN send back MSG_FIN to tell client that this socket is busy
		//if ack from paired socket and its seq = this ack set flag for ack received?
		//if MSG_FIN reply with ack and if initialized flag = 1 set alarm for 15s to reset everything and set initialized to 0

		//Retransmissions may also be necessary if one end indicates that it did not receive a message that was earlier sent from
		//the another endpoint. This condition is detected through discrepancies between the sender's sequence number and the
		//receiver's acknowledgement number. Refer to the slides for more information about sequence numbers and acknowledgement
		//numbers.
		if(header->message_type == MSG_SYN ) {
            response->protocol = PROTOCOL_MINISTREAM;
            pack_address(response->destination_address,this_network_address);
            pack_address(response->source_address,myaddress);
            pack_unsigned_short(response->destination_port,this_port);
            pack_unsigned_short(response->source_port,socket->port_number);
            response->message_type = MSG_FIN;
            pack_unsigned_int(response->seq_number, socket->seq_number);
            pack_unsigned_int(response->ack_number, socket->ack_number);
            network_send_pkt((unsigned int*)(response->destination_address),sizeof(struct mini_header_reliable),(char*)header,0,NULL);
            return;
		}
		else if (header->message_type == MSG_ACK &&
        this_port==socket->remote_port &&
        (network_compare_network_addresses(this_network_address,socket->remote_address)!=0)) {
            if(*header->ack_number == socket->seq_number) {
                socket->send_ack_received = 1;
            }
            return;
		}
		else if (header->message_type == MSG_FIN &&
        this_port==socket->remote_port &&
        (network_compare_network_addresses(this_network_address,socket->remote_address)!=0)) {
            response->protocol = PROTOCOL_MINISTREAM;
            pack_address(response->destination_address,this_network_address);
            pack_address(response->source_address,myaddress);
            pack_unsigned_short(response->destination_port,this_port);
            pack_unsigned_short(response->source_port,socket->port_number);
            response->message_type = MSG_ACK;
            pack_unsigned_int(response->seq_number, socket->seq_number);
            pack_unsigned_int(response->ack_number, socket->ack_number);
            network_send_pkt((unsigned int*)response->destination_address,sizeof(struct mini_header_reliable),(char*)header,0,NULL);
            socket->initialized = 0;
            //ALARM?
            return;
            }
	}
}
// CLIENT
void handle_SYNACK (minisocket_t socket, mini_header_reliable_t header){ //-1
	//if MSG_SYNACK -> if from same server and it seq = this ack + 1 if so set this ack = seq and reply with
	//MSG_ACK and set flag to initialize /set_handle(socket, -2)
	//if it's a MSG_FIN set socket's socket_busy flag to 1, server will be
	//checking this flag during initialization and return SOCKET_BUSY error
        return;
}
// SERVER AND CLIENT ARE NOW PAIRED
void handle_control_client (minisocket_t socket, mini_header_reliable_t header){ //-2
	//if ack from paired socket if its seq = this ack set flag for ack received?
	//if MSG_FIN reply with ack and if initialized flag = 1 set alarm for 15s to reset everything and set initialized to 0

	//Retransmissions may also be necessary if one end indicates that it did not receive a message that was earlier sent from
	//the another endpoint. This condition is detected through discrepancies between the sender's sequence number and the
	//receiver's acknowledgement number. Refer to the slides for more information about sequence numbers and acknowledgement
	//numbers.
        return;
}
// END OF CONTROL FLOW FUNCTIONS

// BELOW IS EXPOSED IN HEADER FOR NETWORK HANDLER TO DIRECTLY USE
void handle_data (minisocket_t socket, mini_header_reliable_t header){

	// check and see if from paired socket (and that initialized flag is 1) (if initialized flag is 0, but we're the correctly
	// paired socket, the handshake ack was lost, so this current packet represents that ack, so set initialized flag to 1)
	// otherwise if initialized is 1 and this is a paired socket add to queue adjust queue sema reply with
	// ack and adjust seq/ack number
        return;
}

minisocket_t get_socket(int port_number){
	if (port_number >= client_upperbound || port_number < 0){
		printf("ERROR: get_socket");
		return NULL;
	}
	return ports[port_number];
}
// Set the socket's control handle function to specific function
void set_handle (minisocket_t socket, int state) {
	switch (state)
	{
	case 1:
		socket->handle = &handle_SYN;
		break;
	case 2:
		socket->handle = &handle_control_server;
		break;
	case -1:
		socket->handle = &handle_SYNACK;
		break;
	case -2:
		socket->handle = &handle_control_client;
		break;
	}
}

// alarm function to wakeup the sending thread for retransmission
void send_alarm_helper(void* sema){
	semaphore_V ((semaphore_t) sema);
}

/* Initializes the minisocket layer. */
void minisocket_initialize()
{
	// init the big array

}

/*
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_server_create(int port, minisocket_error *error)
{
	minisocket_t this_socket;
	mini_header_reliable_t synack;
	network_address_t temp;
	//int bytes_sent;
	int timeout;
	int loop_nums = 0;
	if (port < 0 || port >= server_upperbound) {
		*error = SOCKET_INVALIDPARAMS;
		return NULL;
	}
	if (ports[port] != NULL){
		*error = SOCKET_PORTINUSE;
		return NULL;
	}
	// create socket
	this_socket    		          	 = (minisocket_t)malloc(sizeof(struct minisocket));
	this_socket->port_number  		 = port;
	this_socket->seq_number       	 = 0;
	this_socket->ack_number       	 = 0;
	//flags, for communication with network_handler
	this_socket->initialized		 = 0;
	this_socket->socket_busy         = 0;
	this_socket->send_ack_received   = 0;
	//
	this_socket->packet_queue        = queue_new();
	this_socket->server_waiting		 = semaphore_create();
	this_socket->receive_sema        = semaphore_create();
	this_socket->outer_receieve_sema = semaphore_create();
	semaphore_initialize(this_socket->outer_receieve_sema,1);
	this_socket->send_sema 		  	 = semaphore_create();
	this_socket->outer_send_sema 	 = semaphore_create();
	semaphore_initialize(this_socket->outer_send_sema,1);
	this_socket->handle 			 = handle_SYN;
	// add to socket array
	ports[port] = this_socket;
	// block on receiving SYN
	semaphore_P(this_socket->server_waiting);
	// network handler wakes up this thread and supplies it with address and port of client
	// increment seq to 1 create a MSG_SYNACK
	this_socket->seq_number+=1;
	// create a MSG_SYNACK
	synack = (mini_header_reliable_t)malloc(sizeof(struct mini_header_reliable));
	network_get_my_address(temp);
	synack->protocol = PROTOCOL_MINISTREAM;
    pack_address(synack->source_address,temp);
    pack_address(synack->destination_address,this_socket->remote_address);
    pack_unsigned_short(synack->source_port,(unsigned short)this_socket->port_number);
    pack_unsigned_short(synack->destination_port,(unsigned short)this_socket->remote_port);
	synack->message_type = MSG_SYNACK;
	pack_unsigned_int(synack->seq_number,(unsigned int)this_socket->seq_number);
    pack_unsigned_int(synack->ack_number,(unsigned int)this_socket->ack_number);
	// send and retransmit till either timeout (where the server should reset the port and go back to listening
	// mode, eg set handle back to handle_syn and wipe stored address and port) or initialized flag is raised)
//	set timeout to 100ms
	timeout = 100;
//	LOOP while (initialized not raised)
	while (this_socket->initialized == 0) {
		loop_nums += 1;
//		if has looped 7 times server should reset the port and go back to listening mode
		if (loop_nums == 8){
			loop_nums = 1;
			timeout = 100;
			this_socket->initialized = 0;
			this_socket->handle = &handle_SYN;
			network_address_blankify(this_socket->remote_address);
			this_socket->remote_port   = 0;
			semaphore_P(this_socket->server_waiting);
			pack_address(synack->destination_address,this_socket->remote_address);
   			pack_unsigned_short(synack->destination_port,(unsigned short)this_socket->remote_port);
		}
//  	register alarm to V send_lock after timeout
		register_alarm(timeout,send_alarm_helper,(void*)this_socket->send_sema);
//  	network_send
		network_send_pkt(this_socket->remote_address,
            sizeof(*synack), (char*)synack,
            0, NULL);
//		P send_lock:
		semaphore_P(this_socket->send_sema);
//		double timeout
		timeout = timeout * 2;
	}
	free(synack);
	// CLIENT AND SERVER ARE NOW LINKED
	return this_socket;
}


// returns next available client port, -1 if none available
int find_next_client_port() {
    int counter = server_upperbound;
    if (current_client == NULL){
        current_client = (int*)malloc(sizeof(int));
        *current_client = server_upperbound;
    }
    if (*current_client < client_upperbound) { //so max val is 65535
        current_client++;
        return *current_client-1;
    }
    else {
	    for (;counter<client_upperbound;counter++){
	        if (ports[counter] == NULL){
	            return counter;
	        }
	    }
    }
    printf("ERROR: no available client ports\n");
    return -1;
}

/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine.
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error)
{
    minisocket_t this_socket;
    mini_header_reliable_t syn;
    network_address_t temp;
    //int bytes_sent; // UNUNSED
	int timeout;
	int loop_nums = 0;
    // generate local port number, return appropriate error if none available
    int this_port_number = find_next_client_port();
    if (this_port_number == -1){
    	*error = SOCKET_NOMOREPORTS;
    	return NULL;
    }
    this_socket    		          	 = (minisocket_t)malloc(sizeof(struct minisocket));
    this_socket->port_number  		 = this_port_number;
	// set remote addr and port
    network_address_copy(addr,this_socket->remote_address);
    this_socket->remote_port         = port;
	// set seq = 1
	this_socket->seq_number       	 = 1;
	this_socket->ack_number          = 0;
	//flags, for communication with network_handler
	this_socket->send_ack_received   = 0;
	this_socket->initialized		 = 0;
	this_socket->socket_busy         = 0;
	//
	this_socket->packet_queue        = queue_new();
	this_socket->server_waiting      = semaphore_create();
	this_socket->receive_sema        = semaphore_create();
	this_socket->outer_receieve_sema = semaphore_create();
	semaphore_initialize(this_socket->outer_receieve_sema,1);
	this_socket->send_sema 		     = semaphore_create();
	this_socket->outer_send_sema     = semaphore_create();
	semaphore_initialize(this_socket->outer_send_sema,1);
    this_socket->handle 	= &handle_SYNACK;
	// add to socket array
	ports[this_port_number] = this_socket;
	// create MSG_SYN
	syn = (mini_header_reliable_t)malloc(sizeof(struct mini_header_reliable));
	network_get_my_address(temp);
	syn->protocol = PROTOCOL_MINISTREAM;
    pack_address(syn->source_address,temp);
    pack_address(syn->destination_address,this_socket->remote_address);
    pack_unsigned_short(syn->source_port,(unsigned short) this_socket->port_number);
    pack_unsigned_short(syn->destination_port,(unsigned short) this_socket->remote_port);
	syn->message_type = MSG_SYN;
	pack_unsigned_int(syn->seq_number,(unsigned int) this_socket->seq_number);
    pack_unsigned_int(syn->ack_number,(unsigned int) this_socket->ack_number);
	// send MSG_SYN 7 times until initialized == 1, if socket_busy is 1 return SOCKET_BUSY and reset remote addr, port
	// if none after 7 return SOCKET_NOSERVER and reset remote addr and port
//	set timeout to 100ms
	timeout = 100;
//	LOOP while (initialized not raised)
	while (this_socket->initialized == 0) {
		loop_nums += 1;
//		if has looped 7 times client should return SOCKET_BUSY reset and return
		if (loop_nums == 8){
			*error = SOCKET_NOSERVER;
			network_address_blankify(this_socket->remote_address);
			this_socket->remote_port = 0;
			return NULL;
		}
		if (this_socket->socket_busy){
			*error = SOCKET_BUSY;
			network_address_blankify(this_socket->remote_address);
			this_socket->remote_port = 0;
			return NULL;
		}
//  	register alarm to V send_lock after timeout
		register_alarm(timeout,send_alarm_helper,(void*)this_socket->send_sema);
//  	network_send
		network_send_pkt(this_socket->remote_address,
            sizeof(*syn), (char*)syn,
            0, NULL);
//		P send_lock:
		semaphore_P(this_socket->send_sema);
//		double timeout
		timeout = timeout * 2;
	}
	free(syn);
	// CLIENT AND SERVER ARE NOW LINKED
	return this_socket;
}

/*
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * the message.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes. Sets the
 *               error code and returns -1 if an error is encountered.
 */
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error)
{
	mini_header_reliable_t header;
	network_address_t temp;
	int bytes_sent;
	int timeout;
	int loop_nums = 0;
	if (socket->initialized == 0){ //thread is unitinitialized or dying
		*error = SOCKET_SENDERROR;
		return -1;
	}
	if (len == 0){
		return 0;
	}
	// increment this socket's seq number
	socket->seq_number += 1;
    network_get_my_address(temp);
    //create the header (miniheader.h has type def)
    header = (mini_header_reliable_t)malloc(sizeof(struct mini_header_reliable));
    header->protocol = PROTOCOL_MINISTREAM;
    pack_address(header->source_address,temp);
    pack_address(header->destination_address,socket->remote_address);
    pack_unsigned_short(header->source_port,(unsigned short) socket->port_number);
    pack_unsigned_short(header->destination_port,(unsigned short) socket->remote_port);
	header->message_type = MSG_ACK;
	pack_unsigned_int(header->seq_number,(unsigned int) socket->seq_number);
    pack_unsigned_int(header->ack_number,(unsigned int) socket->ack_number);
    // send a segment if packet is too big
    if (len + sizeof(*header) > MAX_NETWORK_PKT_SIZE){ // if too big send max size;
    	len = MAX_NETWORK_PKT_SIZE - sizeof(*header);
    }
	//	P outer: to force one outgoing message at a time
	semaphore_P(socket->outer_send_sema);
//		set timeout to 100ms
		timeout = 100;
//		LOOP while (ack not received) (need flag to see if acked or can check ack/seq number?)
		while (socket->send_ack_received == 0) {
			loop_nums += 1;
//			if has looped 7 times set error to appropriate val and return succesful bytes transmitted
			if (loop_nums == 8){
				*error = SOCKET_SENDERROR;
				free(header);
				return -1;
			}
//  		register alarm to V send_lock after timeout
			register_alarm(timeout,send_alarm_helper,(void*)socket->send_sema);
//  		network_send
			bytes_sent = network_send_pkt(socket->remote_address,
                 sizeof(*header), (char*)header,
                 len, msg);
//			P send_lock:
			semaphore_P(socket->send_sema);
//			double timeout
			timeout = timeout * 2;
		}
		socket->send_ack_received = 0;
		free(header);
	semaphore_V(socket->outer_send_sema);
	return bytes_sent;
}

/*
 * A thread asks to receive a message from the other end of the socket. Blocks until
 * some data is received (which can be smaller than max_len bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error)
{
	minimsg_t temp;
	if (socket->initialized == 0){ //thread is unitinitialized or dying
		*error = SOCKET_RECEIVEERROR;
		return -1;
	}
	semaphore_P(socket->outer_receieve_sema);
		semaphore_P(socket->receive_sema);
			queue_dequeue(socket->packet_queue,(void**)&temp);
		semaphore_V(socket->receive_sema);
	semaphore_V(socket->outer_receieve_sema);
	if (sizeof(*temp) > max_len){
		*error = SOCKET_OUTOFMEMORY; //maybe just receive error?
		return -1;
	}
	msg = temp;
	return sizeof(*temp);
}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{
	// Set initialized to 0 and send FIN packet to socket and retransmit waiting for an ack received back, upon ack or after 7, reset everything
	// semaphore_wake_all(data_available);
	// after fin receives/sends on this socket should fail
        return;
}
