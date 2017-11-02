/*
* networkLayer.c
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

#include "concurrentFIFOQueue.h"
#include "transportLayer.h"
#include "networkLayer.h"
#include "subnetsupport.h"
#include "subnet.h"
#include "fifoqueue.h"
#include "debug.h"

networkAddress thisNetworkAddress;
NL_RoutingTable routingTable;

void initialize_networkLayer(int stationID) {
  thisNetworkAddress = stationID;
}

void networkLayer() {
  //Since this network layer isn't 'fake', it has no condition to stop and relies on the above layer to handle this.
  long int events_we_handle = network_layer_allowed_to_send | data_for_network_layer | NL_SendingQueueOffer;
  event_t event;
  
  ConcurrentFifoQueue receivingQueue = CFQ_Init();
  ConcurrentFifoQueue sendingQueue = CFQ_Init();
  ConcurrentFifoQueue *offer;
  FifoQueueEntry e;
  TL_OfferElement o;
  
  datagram d; //Scratch variable
  
  logLine(succes, "<DEBUGGING> NL Started.\n");
  
  while(true) {
    logLine(trace, "NL: Waiting for signals.\n");
    Wait(&event, events_we_handle);
    
    switch(event.type) {
      case network_layer_allowed_to_send:
        
        break;
      case data_for_network_layer:
        
        break;
      case NL_SendingQueueOffer://NL_OfferSendingQueue:
        logLine(debug, "NL: Received signal NL_SendingQueueOffer.\n");
        offer = (ConcurrentFifoQueue*) event.msg;
        logLine(trace, "NL: Received offer queue.\n");
        //Assumes that the queue has already been locked by the offerer. The offerer intends only this process to access the queue and renounces its own access.
        
        //Turn all segments into datagrams
        while(EmptyFQ(offer->queue) == 0) {
          logLine(trace, "NL: Handling element.\n");
          e = DequeueFQ(offer->queue);
          o = *((TL_OfferElement*) (e->val));
          
          d.type = DATAGRAM;
          d.reserved = 0;
          d.payloadsize = MAX_PAYLOAD;
          
          d.src = thisNetworkAddress;
          d.dest = o.receiver;
          
          d.payload = o.seg;
          
          logLine(info, "NL: Received from TL: %s\n", d.payload.data);
        }
        
        logLine(trace, "NL: Releasing locks.\n");
        Unlock(offer->lock); //Done with this queue, allow access to it again.
        Unlock(sendingQueue.lock);
        logLine(debug, "NL: Finished handling signal NL_SendingQueueOffer.\n");
        break;
      
    }
  }
}

//Offer the queue 'offer' to the network layer.
//The queue gets locked and the caller of the function is no longer allowed to access it until it is unlocked by the network layer again.
void NL_OfferSendingQueue(ConcurrentFifoQueue *offer) {
  Lock(offer->lock);
  
  Signal(NL_SendingQueueOffer, (void*) offer);
}


