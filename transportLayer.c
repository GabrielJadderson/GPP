
#include "events.h"
#include "concurrentFIFOQueue.h"

#include "transportLayer.h"
#include "networkLayer.h"

void fake_transportLayer() {
  
  //char* buffer = "HEH.\n";
  ConcurrentFifoQueue *offer;
  ConcurrentFifoQueue *q = malloc(sizeof(ConcurrentFifoQueue));
  *q = CFQ_Init();
  
  /*int wtf = Trylock(q->lock);
  logLine(succes, "TL: Trylock=%d\n", wtf);
  
  Unlock(q->lock);
  wtf = Trylock(q->lock);
  logLine(succes, "TL: Trylock=%d\n", wtf);
  Unlock(q->lock);*/
  
  long int events_we_handle = TL_ReceivingQueueOffer;
  event_t event;
  
  FifoQueueEntry e;
  TL_OfferElement o;
  
  TL_OfferElement *elem;
  /*elem.otherHostAddress = 42;
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
  
  Unlock(q.lock);*/
  
  //Lock (q->lock);
  #define TL_NUM_HEH 100
  int i = 1;
  while(i <= TL_NUM_HEH) {
    elem = malloc(sizeof(TL_OfferElement));
    
    elem->otherHostAddress = 42;
    if(ThisStation == 1) {
      elem->otherHostAddress = 212;
    } else if(ThisStation == 2) {
      elem->otherHostAddress = 111;
    }
    
    char a = i/100;
    if(i < 100) {a = ' ';} else {a += 48;}
    char b = (i%100)/10;
    if(i < 10) {b = ' ';} else {b += 48;}
    char c = (i%10)+48;
    
    //logLine(succes, "11111 TL: Making packet with a, b and c: (i=%d) a@(%d, %c) b@(%d, %c) c@(%d, %c)\n", i, a, a, b, b, c, c);
    
    elem->seg.data[0] = 'H';
    elem->seg.data[1] = 'E';
    elem->seg.data[2] = 'H';
    elem->seg.data[3] = ':';
    elem->seg.data[4] =  a ;
    elem->seg.data[5] =  b ;
    elem->seg.data[6] =  c ;
    elem->seg.data[7] = '\0';
    
    /*elem.seg.data[0] = 'A';
    elem.seg.data[1] = 'S';
    elem.seg.data[2] = 'D';
    elem.seg.data[3] = 'F';
    elem.seg.data[4] = 'G';
    elem.seg.data[5] = 'H';
    elem.seg.data[6] = 'I';
    elem.seg.data[7] = '\0';*/
    
    EnqueueFQ( NewFQE( (void *) elem ), q->queue );
    i++;
  }
  //Unlock(q->lock);
  
  
  logLine(trace, "TL: Offering queue.\n");
  if (ThisStation == 2)
    NL_OfferSendingQueue(q); //We can do this whenever really as it isn't based on a 1:1 signals-handing-out-information thing.
  
  /*wtf = Trylock(q->lock);
  logLine(succes, "TL: Trylock2=%d\n", wtf);

  Unlock(q->lock);
  wtf = Trylock(q->lock);
  logLine(succes, "TL: Trylock2=%d\n", wtf);
  Unlock(q->lock);*/
  
  //Lock(q->lock); //Should no longer be used.
  
  /*while(Trylock(q.lock) != 0) {
    logLine(trace, "TL: Waiting for unlocking of queue.\n");
    sleep(1);
  }
  
  logLine(succes, "TL: NL Released the sending queue.\n");
  
  Stop();*/
  
  int numReceivedPackets = 0;
  
  while(true) {
    logLine(trace, "TL: Waiting for signals.\n");
    logLine(succes, "TL: Lockstatus=%d\n", q->used);
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
          //logLine(succes, "\n                    TL: Received from host with address %d: '%s'\n\n", o.otherHostAddress, o.seg.data);
        }
        
        logLine(trace, "TL: Releasing locks.\n");
        //Unlock(offer->lock);
        //sleep(2); //TEMPORARY!!!
        //Stop();
        
        numReceivedPackets++;
        
        break;
    }
    
    if(numReceivedPackets >= TL_NUM_HEH) {
      /*logLine(succes, "Received all packets.\n");
      if(ThisStation == 1) {
        sleep(2);
        Stop();
      } else {
        NL_OfferSendingQueue(&q);
      }*/
      /*Unlock(q->lock);
      int huh = Trylock(q->lock);
      logLine(succes, "TL: q lock state: %d\n", Trylock(q->lock));
      if(huh == 16) {
         Unlock(q->lock);
      }
      Unlock(q->lock);
      huh = Trylock(q->lock);
      logLine(succes, "TL: q lock state: %d\n", Trylock(q->lock));
      if(huh == 0) {
        Unlock(q->lock);
      }*/
      
      sleep(2);
      Stop();
    }
    
    
  }
  
}

void TL_OfferReceivingQueue(ConcurrentFifoQueue *offer) {
  //Lock(offer->lock);
  
  Signal(TL_ReceivingQueueOffer, (void*) offer);
}

