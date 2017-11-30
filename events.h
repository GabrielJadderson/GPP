/*
* events.h
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

#ifndef EVENTS_H_
#define EVENTS_H_

/* Legend:
*  LL: Link Layer
*  NL: Network Layer
*  TL: Transport Layer
*  AL: Application Layer
*/

/*
*   Event Definitions
*/

// LL <-> NL
#define network_layer_allowed_to_send  0x00000004 //LL->NL: LL can receive an element from NL
#define network_layer_ready            0x00000008 //NL->LL: LL can retrieve an element from NL's outbound queue //DEPRECATED
#define data_for_network_layer         0x00000010 //LL->NL: NL can retrieve an element from its own inbound queue
#define LL_SendingQueueOffer           0x00000020 //NL->LL: Offer LL a queue of elements for LL to forward. The offered concurrentFifoQueue is assumed locked.

// NL <-> TL
#define NL_SendingQueueOffer           0x00000040 //TL->NL: Offer NL a queue of elements for NL to forward. The offered concurrentFifoQueue is assumed locked.
#define TL_ReceivingQueueOffer         0x00000080 //NL->TL: Offer TL a queue of elements for it to receive. The offered concurrentFifoQueue is assumed locked.

// TL <-> AL
//#define TL_RequestSocket               0x00000000 //AL->TL: Application in AL signals to TL that it requests a socket.
#define AL_Listen                      0x00000100 //AL->TL: Application in AL signals to TL that it is waiting for an incoming connection. Should block the application.
#define AL_Connect                     0x00000200 //AL->TL: Application sends a connection request to another application on another host.
#define AL_Disconnect                  0x00000400 //AL->TL: Application sends a disconnect notice to another application on another host.
#define AL_Send                        0x00000800 //AL->TL: Application sends data to TL for sending to an application on another host.
#define AL_Receive                     0x00001000 //AL->TL: Application receives data from TL.

#endif /* include guard: EVENTS_H_ */

