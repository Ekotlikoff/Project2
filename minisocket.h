#ifndef __MINISOCKETS_H_
#define __MINISOCKETS_H_

/*
 *	Definitions for minisockets.
 *
 *      You should implement the functions defined in this file, using
 *      the names for types and functions defined here. Functions must take
 *      the exact arguments in the prototypes.
 *
 *      miniports and minisockets should coexist.
 */

#include "network.h"
#include "minimsg.h"
#include "miniheader.h"

struct minisocket
{
	int               port_number;
	network_address_t remote_address;
	int 			  remote_port;
	int 			  in_use;
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
	void (*handle)(struct minisocket*, mini_header_reliable_t);//pointer to current control flow handling function
};

typedef struct minisocket* minisocket_t;

typedef enum minisocket_error minisocket_error;

enum minisocket_error {
  SOCKET_NOERROR=0,
  SOCKET_NOMOREPORTS,   /* ran out of free ports */
  SOCKET_PORTINUSE,     /* server tried to use a port that is already in use */
  SOCKET_NOSERVER,      /* client tried to connect to a port without a server */
  SOCKET_BUSY,          /* client tried to connect to a port that is in use */
  SOCKET_SENDERROR,
  SOCKET_RECEIVEERROR,
  SOCKET_INVALIDPARAMS, /* user supplied invalid parameters to the function */
  SOCKET_OUTOFMEMORY    /* function could not complete because of insufficient memory */
};

// type of handle control packet function, each socket has one of these
//typedef void (*handle)(minisocket_t, mini_header_reliable_t); /* pointer to current handling function, each socket should have one eh'? */

extern minisocket_t get_socket(int port_number);

// returns this sockets current control flow function to be used on control flow packets
extern void (*get_handle_function(minisocket_t socket))(minisocket_t, mini_header_reliable_t);

// used by network handler to handle data packet
extern void handle_data(minisocket_t socket,mini_header_reliable_t header,char* buf);

/* Initializes the minisocket layer. */
void minisocket_initialize();

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
minisocket_t minisocket_server_create(int port, minisocket_error *error);

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
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error);

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
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error);

/*
 * Receive a message from the other end of the socket. Blocks until max_len
 * bytes or a full message is received (which can be smaller than max_len
 * bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error);

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket);

#endif /* __MINISOCKETS_H_ */
