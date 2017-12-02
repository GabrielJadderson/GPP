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

typedef struct {
  unsigned int connectionState;
  FifoQueue messageQueue;
} TLConnection;

#define MAX_CONNECTIONS 4
typedef struct {
  unsigned int valid:1;
  transPORT port;
  TLConnection connections[MAX_CONNECTIONS];
} TLSocket;

typedef struct {
  transPORT port;
  TLSocket *sock;
} TLSockReq;

typedef struct {
  unsigned int is_first:1;
  int seqMsg;
  int seqPayload;
  transPORT senderport;
  transPORT receiverport;
  
  payload msg;
} TL_Segment;

typedef struct {
  networkAddress otherHostAddress;
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
