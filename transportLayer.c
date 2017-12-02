
#include "events.h"
#include "concurrentFIFOQueue.h"

#include "transportLayer.h"
#include "networkLayer.h"

void transportLayer() {
  ConcurrentFifoQueue *offer;
  ConcurrentFifoQueue *q = malloc(sizeof(ConcurrentFifoQueue));
  *q = CFQ_Init();
  
  FifoQueueEntry e;
  TL_OfferElement o;
  
  TL_OfferElement *elem;
  
  #define TL_NUM_HEH 10
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
    
    EnqueueFQ( NewFQE( (void *) elem ), q->queue );
    i++;
  }
  
  
  logLine(trace, "TL: Offering queue.\n");
  //if (ThisStation == 2)
    NL_OfferSendingQueue(q); //We can do this whenever really as it isn't based on a 1:1 signals-handing-out-information thing.
  
  
  //----------------------------------------------------------------------------------------------------------------------------------------------------------
  
  #define NUM_MAX_SOCKETS 4
  #define SOCKET_ANY NUM_MAX_SOCKETS
  TLSocket *sockets[NUM_MAX_SOCKETS]; //the transPORT values correspond to indices in this array. SOCKET_ANY is any socket.
  
  for (int i = 0; i < NUM_MAX_SOCKETS; i++) {
    sockets[i] = NULL;
  }
  
  
  
  
  
  
  
  
  
  TLSocket** socket;
  TLSockReq* req;
  
  int numReceivedPackets = 0;
  
  event_t event;
  long int events_we_handle = TL_ReceivingQueueOffer | TL_SocketRequest;
  while(true) {
    logLine(trace, "TL: Waiting for signals.\n");
    //logLine(succes, "TL: Lockstatus=%d\n", q->used);
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
      case TL_SocketRequest:
        //logLine(succes, "TL: start\n");
        
        req = (TLSockReq*) event.msg;
        
        //TODO: [PJ] Should first check if the port is available.
        
        if(req->port == SOCKET_ANY) {
          for(int i = 0; i < NUM_MAX_SOCKETS; i++) {
            if(sockets[i] == NULL) {
              req->port = i;
              break;
            }
          }
        } else {
          if(sockets[req->port] != NULL || req->port >= NUM_MAX_SOCKETS) { // [PJ] Port taken/port invalid. Return an invalid port. Everything else than the valid field can be anything.
            socket = malloc(sizeof(TLSocket*));
            (*socket) = malloc(sizeof(TLSocket));
            (*socket)->valid = 0;
            req->sock = (*socket);
            break;
          }
        }
        
        sockets[req->port] = malloc(sizeof(TLSocket*));
        
        (sockets[req->port])->port = req->port; //Because the information being shared with the application.
        (sockets[req->port])->valid = 1; //The port is valid.
        
        req->sock = sockets[req->port];
        
        /*
        socket = malloc(sizeof(TLSocket*));
        (*socket) = malloc(sizeof(TLSocket));
        
        logLine(succes, "TL: Requested port: %d\n", req->port);
        (*socket)->port = req->port;
        (*socket)->valid = 1;
        
        logLine(succes, "TL: assigned port: %d\n", (*socket)->port);
        req->sock = (*socket);
        */
        
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

TLSocket* TL_RequestSocket(transPORT port) {
  //logLine(succes, "TLRS: start\n");
  TLSockReq* reqp = malloc(sizeof(TLSockReq));
  reqp->port = port;
  reqp->sock = NULL;
  
  //logLine(succes, "TLRS: Request*: %p\n", reqp);
  Signal(TL_SocketRequest, reqp);
  
  //logLine(succes, "TLRS: Beginning loop.\n");
  // [PJ] For some reason it needs to perform an action in here. Must be due to some optimization at compiletime with socketpp->sock being initialized as NULL.
  // Make sure to choose something that can't be optimized away, like a statement with no effect or anything like that.
  while(reqp->sock==NULL) {logLine(trace, "waitin'\n");} 
  
  //logLine(succes, "POOOOOORT: %d\n", (reqp->sock)->port);
  
  TLSocket* ret = reqp->sock;
  free(reqp);
  return ret;
}

