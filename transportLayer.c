
#include "events.h"
#include "concurrentFIFOQueue.h"

#include "transportLayer.h"
#include "networkLayer.h"

void fake_transportLayer() {
  
  //char* buffer = "HEH.\n";
  ConcurrentFifoQueue *offer;
  ConcurrentFifoQueue q = CFQ_Init();
  
  long int events_we_handle = TL_ReceivingQueueOffer;
  event_t event;
  
  FifoQueueEntry e;
  TL_OfferElement o;
  
  TL_OfferElement elem;
  elem.otherHostAddress = 42;
  if(ThisStation == 1) {
    elem.otherHostAddress = 212;
  } else if(ThisStation == 2) {
    elem.otherHostAddress = 111;
  }
  
  //elem.seg = "HEH.\n";
  elem.seg.data[0] = 'H';
  elem.seg.data[1] = 'E';
  elem.seg.data[2] = 'H';
  elem.seg.data[3] = '.';
  elem.seg.data[4] = '\0';
  
  Lock(q.lock);
  
  EnqueueFQ( NewFQE( (void *) &elem ), q.queue );
  
  Unlock(q.lock);
  
  logLine(trace, "TL: Offering queue.\n");
  NL_OfferSendingQueue(&q); //We can do this whenever really as it isn't based on a 1:1 signals-handing-out-information thing.
  
  /*while(Trylock(q.lock) != 0) {
    logLine(trace, "TL: Waiting for unlocking of queue.\n");
    sleep(1);
  }
  
  logLine(succes, "TL: NL Released the sending queue.\n");
  
  Stop();*/
  
  while(true) {
    logLine(trace, "TL: Waiting for signals.\n");
    Wait(&event, events_we_handle);

    switch(event.type) {
      case TL_ReceivingQueueOffer:
        logLine(debug, "TL: Received signal TL_ReceivingQueueOffer.\n");
        offer = (ConcurrentFifoQueue*) event.msg;
        logLine(trace, "TL: Received offer queue.\n");
        //Assumes that the queue has already been locked by the offerer. The offerer intends only this process to access the queue and renounces its own access.
        
        //Handle incoming messages
        while(EmptyFQ(offer->queue) == 0) {
          logLine(trace, "TL: Handling element.\n");
          e = DequeueFQ(offer->queue);
          o = *((TL_OfferElement*) (e->val));
          
          logLine(succes, "TL: Received from host with address %d: '%s'\n", o.otherHostAddress, o.seg.data);
        }
        
        logLine(trace, "TL: Releasing locks.\n");
        Unlock(offer->lock);
        sleep(2); //TEMPORARY!!!
        Stop();
        
        break;
      /*case NL_SendingQueueOffer://NL_OfferSendingQueue:
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
        break;*/
    
    }
    
    
    
  }
  
}

void TL_OfferReceivingQueue(ConcurrentFifoQueue *offer) {
  Lock(offer->lock);
  
  Signal(TL_ReceivingQueueOffer, (void*) offer);
}

