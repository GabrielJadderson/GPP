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
#define network_layer_ready            0x00000008 //NL->LL: LL can retrieve an element from NL's outbound queue
#define data_for_network_layer         0x00000010 //LL->NL: NL can retrieve an element from its own inbound queue

// NL <-> TL



