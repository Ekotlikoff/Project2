/*
 *	Implementation of minisockets.
 */
#include <stdlib.h>
#include <stdio.h>
#include "minisocket.h"
#include "miniheader.h"


#define client_upperbound 65536
#define server_upperbound 32768
#ifndef NULL
#define NULL 0
#endif

struct minisocket
{
	// socket number
	// initialized = 0 (set to 1 if ack received after SYNACK (SERVER) and 1 if ack sent(client))
	// sema for queue (for receive calls to P on and data_handle to V on) and outer sema so that multiple 
												   //receivers aren't manipulating queue at the same time
	// queue for packets
	// sequence number (when send increment)
	// ack number (when receive increment)
	// send semaphore (and outer sema to make sure only one person is manipulating that sema at a time)
	// control handling function
};

// big array of clients and servers
minisocket_t ports[client_upperbound];

// type of handle control packet function, each socket has one of these
typedef void (*handle)(mini_header_t); /* pointer to current handling function, each socket should have one eh'? */

// SERVER

void handle_SYN (mini_header_t header){ //1
	//check that packet is MSG_SYN, keep in mind this is being done in network handler
	//if so set_handle(port number of this socket, 2) and wakeup the thread blocking on create server
	// and give it address and port of client so that it will send SYN_ACK with updated ack_number and retransmit...
}
void handle_ACK (mini_header_t header){ //2
	//if packet is MSG_SYN again and from the same client reply again with SYNACK, if from another client send MSG_FIN back
	//else if is MSG_ACK notify the socket by setting initialized flag and set_handle(port number of this socket, 3)
}

void handle_control_server (mini_header_t header){ //3
	//if MSG_SYN send back MSG_FIN to tell client that this socket is busy
	//THIS IS NOW DATA TRANSMISSION if ack from paired socket change seq_number /ack_number 

	//Retransmissions may also be necessary if one end indicates that it did not receive a message that was earlier sent from 
	//the another endpoint. This condition is detected through discrepancies between the sender's sequence number and the 
	//receiver's acknowledgement number. Refer to the slides for more information about sequence numbers and acknowledgement 
	//numbers.
}

// CLIENT

void handle_SYNACK (mini_header_t header){ //-1
	//check that packet is MSG_SYNACK from same server if so reply with MSG_ACK and set flag to initialized
	//set_handle(port number of this socket, -2)       if it's a MSG_FIN set 
}

void handle_control_client (mini_header_t header){ //-2
	//THIS IS NOW DATA TRANSMISION if ack from paired socket change seq_number /ack_number 

	//Retransmissions may also be necessary if one end indicates that it did not receive a message that was earlier sent from 
	//the another endpoint. This condition is detected through discrepancies between the sender's sequence number and the 
	//receiver's acknowledgement number. Refer to the slides for more information about sequence numbers and acknowledgement 
	//numbers.
}

// HANDLE DATA, called by network handler if packet is a data packet

void handle_data (mini_header_t header){

	// check and see if from paired socket (and that initialized flag is 1) (if initialized flag is 0, but we're the correctly
	// paired socket, the handshake ack was lost, so this current packet represents that ack, so set initialized flag to 1)
	// otherwise if initialized is 1 and this is a paired socket add to queue adjust queue sema reply with 
	// ack and adjust seq/ack number
}

// Set the socket's control handle function to specific function
void set_handle (minisocket_t socket, int state) {
	if (state == 1){
		handle = handle_SYN;
	}
	else if (state = 2) {
		handle = handle_ACK;
	}
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
	// create socket
	// add to socket array
	//
	// block on receiving SYN
	// 
	// network handler wakes up this thread and supplies it with address and port of client

	// send a MSG_SYNACK (and retransmit till either timeout (where the server should reset the port and go back to listening 
	// mode, eg set handle back to handle_syn and wipe stored address and port) or initialized flag is raised)

	// You have to make sure that sockets are used one-to-one, i.e. one socket will be sending packets to only one other socket. 
	// Any attempt to open a new connection to an already used socket (i.e. socket to which there is another connection already 
	// made) should result in an MSG_FIN packet sent back to the client. The client should report this as a SOCKET_BUSY error.
	// ^ TODO UNDERSTANDING, will this require server to respond with a busy message?  YES!  in interrupt handler of server should 
	// check if server is busy and respond with busy if that's the case
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
	// generate local port number, return appropriate error if none available

	// send MSG_SYN 7 times until reply, if none return SOCKET_NOSERVER

	// if initialized flag set (eg if SYNACK replied) return minisocket

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

//	P outer: to force one outgoing message at a time
//		set timeout to 100ms
//  	keep track of total length of messages sent (taking into account only partial sending because 
//		network send can send partial right?) and loop until total length is
//		sizeof(msg) long
//		LOOP of if total length is sizeof(msg)
//			LOOP while (ack not received) (need flag to see if acked or can check ack/seq number?)
//				loop_nums += 1;
//				if loop_nums == 7 set error to appropriate val and return succesful bytes transmitted
//  			register alarm to V send_lock after timeout
//  			network_send
//				P send_lock:
//					wait
//				double timeout
//			reset timeout to 100
//		

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
	//just P on the outer queue lock and the queue sema and then pop off of data queue and V outer queue lock
}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{
// need to notify all threads blocking on this socket with recieve
// after fin receives/sends on this socket should fail
}
