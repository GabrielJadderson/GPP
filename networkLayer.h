/*
* networkLayer.h
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

#include "concurrentFIFOQueue.h"

#ifndef NL_H_
#define NL_H_

/*
*   Types
*/

// [PJ] Made the following values signed because -1 means unused.
//Address type for devices on the network implementing the network layer. TODO: MOVE TO TRANSPORT LAYER HEADER FILE
typedef signed long networkAddress; //[PJ] Considered making this an unsigned char[4]. Individual component access is unnecessary for this though.

//Neighbour ID for neighbouring devices.
typedef signed int neighbourid;

// Datagram type. Explicitly not typedeffed. More can be added.
enum {DATAGRAM, ROUTERINFO};
/* Documentation:
DATAGRAM: A packet to be delivered to the transport layer.
ROUTERINFO: Router information request, if own addres == dest, send a DATAGRAM to src with information based on the contents of the payload. Unimplemented.
*/

// Datagram struct and its associated values.
#define MAX_PAYLOAD 8
typedef struct { char data[MAX_PAYLOAD]; } payload;

typedef struct  {
  //First 32bit word
  unsigned char type;
  unsigned char reserved; //Currently unused. Exists for expandability and to pad to 32-bit word. Considering a next-header value with additional options as in IPv6.
  unsigned short payloadsize;
  //^
  
  //Two next 32-bit words
  networkAddress src;
  networkAddress dest;
  //^
  
  //Payload
  payload payload;
} datagram;

//Routing table. There can be more elements in this table than there are neighbours and multiple entries can point to the same neighbour.
#define NL_ROUTING_TABLE_SIZE 4
typedef struct {
  networkAddress addresses[NL_ROUTING_TABLE_SIZE];
  neighbourid    neighbourids[NL_ROUTING_TABLE_SIZE];
} NL_RoutingTable;

typedef struct {
  neighbourid otherHostNeighbourid;
  datagram dat;
} NL_OfferElement;

/*
*   Function Predeclarations
*/

//Makes a network address lookup to determine which outgoing neighbour to send to.
neighbourid NL_TableLookup(networkAddress addr);

//The network layer accepts a queue and feeds it into its own sending system (queue(s)). Blocks the queue for the whole duration.
// Note: This function locks the queue and returns, allowing for other tasks to be handled. If NL never unlocks it, it will stay locked.
//Allows the transport layer to better control its sending of data. It can also turn a message into multiple segments and dump them into the queue and send it all with one signal.
//If the transport layer needs to queue up more segments and the queue is in use, then it needs to use another queue in the meantime.
void NL_OfferSendingQueue(ConcurrentFifoQueue *queue);



#endif /* include guard: NL_H_ */
