/*
* transportLayer.h
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

#ifndef TL_H_
#define TL_H_

/*
*   Types
*/

typedef unsigned int transPORT;

//A buffered system like this is used because nothing about the lower layers are assumed except reliable data transfer. As such, the package routing could be arbitrary and give garbled arrival orders if something like load balancing had been implemented.
//Using a linked list to avoid implementation overhead for an array-based implementation.
typedef struct TLMBLL{
  struct TLMBLL *next;
  struct TLMBLL *prev; //Searching will start from this end. It is simply more likely to hit a correct spot faster.
  unsigned char* msg;
  unsigned int msgLen; //The length of msg. Allocated based on the aux field of the first segment.
  unsigned int seqMsg; //The sequence number of the message.
  unsigned int fragmentsRemaining; //The number of fragments yet to arrive. Set by the first segment in the seqPayload field. Decrement when receiving fragments. When it hits 0, all fragments have been received. This method is safe because of rdt in the link layer.
} TLMessageBufferLL;

//Connection management struct.
typedef struct {
  unsigned int valid:1; //This is checked before actual access.
  networkAddress remoteAddress; //Address of the other machine
  transPORT remotePort; //Port of the other application
  TLMessageBufferLL *msgListHead; //Head of the linked list of messages. If this one has fragmentsRemaining == 0, then its msg can be received by the application.
  TLMessageBufferLL *msgListTail; //Tail of the linked list of messages
} TLConnection;

#define MAX_CONNECTIONS 4
typedef struct {
  unsigned int valid:1; //This is checked before actual access. The TL sets this as an indicator for the AL.
  unsigned int listening:1; //This is used to keep track of listening status and when to break out of waiting in the listening function.
  transPORT port; //The port of the socket. The application can check this value to see which port is has been given if it chose any port.
  TLConnection connections[MAX_CONNECTIONS]; //Connections
} TLSocket;

typedef struct {
  transPORT port; //Port that is requested. Can also be a request for any port.
  TLSocket *sock; //The pointer to assign to the address of the returned socket.
} TLSockReq;

//Segment type.
//Last message is infered from the number of bytes in total from the aux field of the first packet, T.
//  (seqPayload+1)*MAX_PAYLOAD >= msgLen => last packet
typedef struct {
  unsigned int is_first:1; //True iff the segment is the first of the message.
  unsigned int seqMsg; //Sequencing number for the messages. No assumptions are made regarding the lower layers and would thereby support random segment arrival order together with seqPayload. (Superficially tested in a closed environment)
  unsigned int seqPayload; //Special case: if is_first is 1, then this value indicates the number of segments the messages is split into, excluding this one.
  unsigned int aux; //is_first: number of bytes of the total message. Used to allocate buffer size. Security? Nope.  For the last message: this is the number of bytes it carries with it.  Otherwise: reserved.
  transPORT senderport; //Port of the application sending the segment.
  transPORT receiverport; //Port of the application receiving the segment.
  
  payload msg; //Actual bytes carried by the segment.
} TL_Segment;

//TL -> NL. TL: Hey NL, send this segment to this address plox!, NL: Okay, buddy!
typedef struct {
  networkAddress otherHostAddress; //Address on the network to send the segment to.
  TL_Segment segment; //Segment to send.
  payload seg; //[PJ] DEPRECATED. FIX IN NL.
} TL_OfferElement;


/*
*   Function Predeclarations
*/

void TL_OfferReceivingQueue(ConcurrentFifoQueue *queue);

//Socket request. Returns NULL on failure.
TLSocket* TL_RequestSocket(transPORT port);
//Function to signal to the TL that the socket is no longer used so it can dispose of the socket.
void discardSocket();

#endif /* include guard: TL_H_ */
