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
  unsigned int msgLen;
  unsigned int seqMsg;
  unsigned int fragmentsRemaining;
} TLMessageBufferLL;

typedef struct {
  unsigned int valid:1;
  networkAddress remoteAddress;
  transPORT remotePort;
  //FifoQueue messageQueue;
  TLMessageBufferLL *msgListHead;
  TLMessageBufferLL *msgListTail;
} TLConnection;

#define MAX_CONNECTIONS 4
typedef struct {
  unsigned int valid:1;
  unsigned int listening:1;
  transPORT port;
  TLConnection connections[MAX_CONNECTIONS];
} TLSocket;

typedef struct {
  transPORT port;
  TLSocket *sock;
} TLSockReq;

//Segment type.
//Last message is infered from the number of bytes in total from the aux field of the first packet, T.
//  (seqPayload+1)*MAX_PAYLOAD >= msgLen => last packet
typedef struct {
  unsigned int is_first:1;
  unsigned int seqMsg;
  unsigned int seqPayload; //Special case: if is_first is 1, then this value indicates the number of segments the messages is split into, excluding this one.
  unsigned int aux; //is_first: number of bytes of the total message. Used to allocate buffer size. Security? Nope. For the last message, this is the number of bytes it carries with it.
  transPORT senderport;
  transPORT receiverport;
  
  payload msg;
} TL_Segment;

typedef struct {
  networkAddress otherHostAddress;
  TL_Segment segment;
  payload seg;
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
