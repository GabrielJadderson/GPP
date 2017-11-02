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

#define ROUTINGTABLEADD(i, a, n) routingTable.addresses[i] = a; routingTable.neighbourids[i] = n;
void initialize_networkLayer(int stationID) {
  switch(stationID) {
    case 1: //Host A
      thisNetworkAddress = 100;
      ROUTINGTABLEADD(0, 212, 0);
      ROUTINGTABLEADD(1, -1, -1);
      ROUTINGTABLEADD(2, -1, -1);
      ROUTINGTABLEADD(3, -1, -1);
      break;
    case 2: //Host B
      thisNetworkAddress = 212;
      ROUTINGTABLEADD(0, 100, 0);
      ROUTINGTABLEADD(1, -1, -1);
      ROUTINGTABLEADD(2, -1, -1);
      ROUTINGTABLEADD(3, -1, -1);
      break;
    case 3: //Router 1
      
      //break;
    case 4: //Router 2
      
      //break;
    default:
      logLine(error, "NL: Network layer initialized for station without a case (%d) - Sending Stop Signal.\n", stationID);
      Stop();
  }
}

//Offer the queue 'offer' to the network layer.
//The queue gets locked and the caller of the function is no longer allowed to access it until it is unlocked by the network layer again.
void NL_OfferSendingQueue(ConcurrentFifoQueue *offer) {
  Lock(offer->lock);
  
  Signal(NL_SendingQueueOffer, (void*) offer);
}

void networkLayerHost() {
  //Since this network layer isn't 'fake', it has no condition to stop and relies on the above layer to handle this.
  long int events_we_handle = network_layer_allowed_to_send | data_for_network_layer | NL_SendingQueueOffer;
  event_t event;
  
  boolean allowedToSendToLL = false;
  
  ConcurrentFifoQueue receivingQueue = CFQ_Init(); //This is concurrent because it is used for communication with the transport layer. The queue is also used for consistence and expandability.
  FifoQueue sendingQueue = InitializeFQ(); //Doesn't need to be concurrent because elements are given with signals.
  ConcurrentFifoQueue *offer;
  FifoQueueEntry e;
  TL_OfferElement *o;
  NL_OfferElement *O; //Using a pointer to create a new one every time.
  
  datagram d; //Scratch variable
  
  logLine(succes, "<DEBUGGING> NL Started.\n");
  
  while(true) {
    logLine(trace, "NL: Waiting for signals.\n");
    Wait(&event, events_we_handle);
    
    switch(event.type) {
      case network_layer_allowed_to_send:
        logLine(trace, "NL: Allowed to send by LL.\n");
        allowedToSendToLL = true; //There might not be an element to send right now. Remember that we can send without getting stuck here.
        break;
      case data_for_network_layer:
        logLine(trace, "NL: Received datagram from LL.\n");
        //We're a host. We don't route packets, we receive them.
        
        d = *((datagram*) (event.msg));
        
        if(d.dest != thisNetworkAddress) {
          logLine(trace, "NL: Datagram was not addressed for this host. Dropping.\n");
          break; //We don't route packets and this wasn't for us.
        }
        
        logLine(trace, "NL: Datagram type (enum): %d.\n", d.type);
        switch(d.type) {
          case DATAGRAM:
            o = malloc(sizeof(TL_OfferElement));
            o->otherHostAddress = d.src;
            o->seg = d.payload;
            
            logLine(trace, "NL: Locking and enqueueing segment for TL.\n");
            Lock(receivingQueue.lock); //TODO: If queue is locked, then use a secondary queue instead. Will also prevent blocking if the upper layer is stalled for some reason.
            EnqueueFQ( NewFQE( (void *) o ), receivingQueue.queue );
            Unlock(receivingQueue.lock);
            logLine(trace, "NL: Offering receiving queue to TL.\n");
            TL_OfferReceivingQueue(&receivingQueue);
            break;
          case ROUTERINFO:
            break; //Currently unused. Dropping.
        }
        
        
        //free(event.msg); //[PJ] Freeing this makes everything explode. It is probably freed internally in the subnet somewhere. This stays to stress this.
        break;
      case NL_SendingQueueOffer://NL_OfferSendingQueue:
        logLine(debug, "NL: Received signal NL_SendingQueueOffer.\n");
        offer = (ConcurrentFifoQueue*) event.msg;
        logLine(trace, "NL: Received offer queue.\n");
        //Assumes that the queue has already been locked by the offerer. The offerer intends only this process to access the queue and renounces its own access.
        
        //Turn all segments into datagrams
        while(EmptyFQ(offer->queue) == 0) {
          logLine(trace, "NL: Handling element from TL.\n");
          e = DequeueFQ(offer->queue);
          o = ((TL_OfferElement*) ValueOfFQE(e));
          
          //logLine(trace, "NL: 11111\n");
          d.type = DATAGRAM;
          d.reserved = 0;
          d.payloadsize = MAX_PAYLOAD;
          
          //logLine(trace, "NL: 22222\n");
          d.src = thisNetworkAddress;
          d.dest = o->otherHostAddress;
          
          //logLine(trace, "NL: 33333\n");
          d.payload = o->seg;
          
          //logLine(trace, "NL: 44444\n");
          O = malloc(sizeof(NL_OfferElement));
          O->otherHostNeighbourid = 0; //TODO: routing table lookup.
          O->dat = d;
          
          //logLine(trace, "NL: 55555\n");
          EnqueueFQ(NewFQE((void *)O), sendingQueue);
          
          logLine(info, "NL: Received from TL: %s\n", d.payload.data);
        }
        
        logLine(trace, "NL: Releasing locks.\n");
        Unlock(offer->lock); //Done with this queue, allow access to it again.
        
        logLine(debug, "NL: Finished handling signal NL_SendingQueueOffer.\n");
        break;
      
    }
    
    //If we are allowed to send, then do so.
    // If the signal was sent in this loop iteration: same as if this was in the case directly
    // otherwise: we received something to send after getting clearance and would have been stuck if this was directly in the case.
    if(allowedToSendToLL && EmptyFQ(sendingQueue) == 0) {
      e = DequeueFQ(sendingQueue); //Extract from queue
      
      Signal(network_layer_ready, ValueOfFQE(e)); //Just pass it directly.
    }
  }
}

