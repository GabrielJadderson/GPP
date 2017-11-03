/*
* events.h
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

/* Legend:
*  LL: Link Layer
*  NL: Network Layer
*  TL: Transport Layer
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


