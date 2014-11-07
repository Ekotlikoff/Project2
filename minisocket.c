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
  int dummy; /* delete this field */
  /* put your definition of minisockets here */
};

minisocket_t ports[client_upperbound];

typedef void (*handle)(mini_header_t); /* pointer to current handling function, each socket should have one eh'? */

void handle_SYN (mini_header_t header){ //1
	//check that packet is MSG_SYN, keep in mind this is being done in network handler
	//if so set_handle(port number of this socket, 2) and wakeup the thread blocking on create server with the SYN_ACK flag
	//raised and give it address and port of client so that it will send SYN_ACK with updated ack_number and retransmit...
}
void handle_ACK (mini_header_t header){ //2
	//if packet is MSG_SYN again reply again with SYNACK
	//else if is MSG_ACK to same port notify the socket that it's been initialized to specific client (unblock thread waiting on it)
	//and set_handle(port number of this socket, 3)
}

void handle_data (mini_header_t header){ //3
	//TODO WHAT SHOULD THIS DO
}

void handle_SYNACK (mini_header_t header){ //-1
	//check that packet is MSG_SYNACK if so reply with MSG_ACK and 
	//set_handle(port number of this socket, -2) TODO WHAT SHOULD THIS NEXT FUNC BE
}

void 
	//-2


void set_handle (minisocket_t, int state) {
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

	// for sockets
	// socket number
	// queue for packets
	// sequence number (when send increment)
	// ack number (when receive increment)
	// send semaphore (and outer sema to make sure only one person is manipulating that sema at a time)

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
	// block on receiving msg
	// if no thread is waiting for a connection on given port, server shouldn't respond.  The client, after 7 retries will 
	// return SOCKET_NOSERVER to the user
	//
	// otherwise, server should send an MSG_SYNACK (and retransmit till either timeout (where the server should reset the port 
	// and go back to listening mode) or receive client's MSG_ACK)

	// You have to make sure that sockets are used one-to-one, i.e. one socket will be sending packets to only one other socket. 
	// Any attempt to open a new connection to an already used socket (i.e. socket to which there is another connection already 
	// made) should result in an MSG_FIN packet sent back to the client. The client should report this as a SOCKET_BUSY error.
	// ^ TODO UNDERSTANDING, will this require server to respond with a busy message?  YES!  in interrupt handler of server should 
	// check if server is busy and respond with busy if that's the case

	// need big array of sockets (for some reason, find this out)

	// network_handler
	// all non data packets are handled directly here

	// multiple ways to handle state, could have enum states or a function pointer called handle that represents what state 
	// you're at. when the state changes update the pointer

	// servers treated like unbound when creating
	// clients treated like bound when creating
	// also the numbers are subject to the same restrictions as port numbers

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

	// send MSG_SYN 7 times until reply, if none return SOCKET_NOSERVER

	// if MSG_SYNACK supplied, reply with MSG_ACK

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
//  	register alarm to V send_lock after timeout
//  	network_send
//		P send_lock:
//			wait

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
	//just block on the receive sema and then pop off of data queue.
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
