
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
          
          
          // Socket Part
          /*
          for(int i = 0; i < NUM_MAX_SOCKETS; i++) {
            if(sockets[i] != NULL && socket[i]->port == o.segment.receiverport) {
              
            }
          }
          */
          /*
          //The port this segment is addressed to is actually valid.
          if(sockets[o.segment.receiverport] != NULL && sockets[o.segment.receiverport]->valid) {
            //[PJ] A search is needed for this one because the data order is indeterministic.
            for(int i = 0; i < MAX_CONNECTIONS; i++) {
              //If there is a hit, go for it. If not, then the message has to be a failure packet or a fraudulent packet.
              if(sockets[o.segment.receiverport]->connections[i].valid //Valid connection.
              && o.otherHostAddress == sockets[o.segment.receiverport]->connections[i].remoteAddress //The sender's address if the address of the connected machine
              && o.segment.senderport == sockets[o.segment.receiverport]->connections[i].remotePort //The sending application's port is the port of the connected application.
              ) {
                //The correct connection has been found. Handle the incoming payload.
                
                TLConnection *con = &(sockets[o.segment.receiverport]->connections[i]);
                
                //New message entirely.
                if(o.segment.is_first) {
                  
                  boolean notAlreadyRegistered = false;
                  
                  for(TLMessageBufferLL* i = con->msgListHead; i != NULL; i = i->next) {
                    if(
                  }
                  
                  if(con->msgListHead == NULL) {
                    con->msgListHead = (TLMessageBufferLL*) malloc(sizeof(TLMessageBufferLL));
                  }
                }
                
                
                break;
              }
            }
          }
          */
          
          // Endof: Socket Part
          
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

void socketCodeTest() {
  
  TL_OfferElement *offer;
  TL_OfferElement o;
  
  TLSocket *sockets[NUM_MAX_SOCKETS]; //the transPORT values correspond to indices in this array. SOCKET_ANY is any socket.
  
  for (int i = 0; i < NUM_MAX_SOCKETS; i++) {
    sockets[i] = NULL;
  }
  
  
  //Register dummy variables.
  
  //Socket opened on port 0 with connection to 111:2
  sockets[0] = malloc(sizeof(TLSocket));
  sockets[0]->valid = 1;
  sockets[0]->listening = 0;
  sockets[0]->port = 0; //Because the hypothetical AL can read this field to determine which port it has gotten.
  sockets[0]->connections[0].valid = 1;
  sockets[0]->connections[0].remoteAddress = 111;
  sockets[0]->connections[0].remotePort = 2;
  sockets[0]->connections[0].msgListHead = NULL;
  sockets[0]->connections[0].msgListTail = NULL;
  
  //A simulated message from 111:2 to ???:0.
  //It's the first message with 0 more fragments.
  offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offer->otherHostAddress = 111;
  offer->segment.is_first = 1;
  offer->segment.seqMsg = 0;
  offer->segment.seqPayload = 0;
  offer->segment.senderport = 2;
  offer->segment.receiverport = 0; //This one should correspond to the open connection 
  //offer->segment.msg.data = "TEST.\n";
  offer->segment.aux = 7;
  offer->segment.msg.data[0] = 'T';
  offer->segment.msg.data[1] = 'E';
  offer->segment.msg.data[2] = 'S';
  offer->segment.msg.data[3] = 'T';
  offer->segment.msg.data[4] = '1';
  offer->segment.msg.data[5] = '.';
  offer->segment.msg.data[6] = '\0';
  offer->segment.msg.data[7] = '\0';
  Signal(TL_ReceivingQueueOffer, offer);
  
  //A simulated message from 111:2 to ???:0.
  //It's the first message with 1 more fragments.
  offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offer->otherHostAddress = 111;
  offer->segment.is_first = 1;
  offer->segment.seqMsg = 2;
  offer->segment.seqPayload = 0;
  offer->segment.senderport = 2;
  offer->segment.receiverport = 0; //This one should correspond to the open connection 
  //offer->segment.msg.data = "TEST.\n";
  offer->segment.aux = 7;
  offer->segment.msg.data[0] = 'T';
  offer->segment.msg.data[1] = 'E';
  offer->segment.msg.data[2] = 'S';
  offer->segment.msg.data[3] = 'T';
  offer->segment.msg.data[4] = '2';
  offer->segment.msg.data[5] = '.';
  offer->segment.msg.data[6] = '\0';
  offer->segment.msg.data[7] = '\0';
  Signal(TL_ReceivingQueueOffer, offer);
  
  //A simulated message from 111:2 to ???:0.
  //It's the first message with 1 more fragments.
  offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offer->otherHostAddress = 111;
  offer->segment.is_first = 1;
  offer->segment.seqMsg = 1;
  offer->segment.seqPayload = 1;
  offer->segment.senderport = 2;
  offer->segment.receiverport = 0; //This one should correspond to the open connection 
  //offer->segment.msg.data = "TEST.\n";
  offer->segment.aux = 8+7;
  offer->segment.msg.data[0] = 'P';
  offer->segment.msg.data[1] = 'A';
  offer->segment.msg.data[2] = 'R';
  offer->segment.msg.data[3] = 'T';
  offer->segment.msg.data[4] = '1';
  offer->segment.msg.data[5] = ' ';
  offer->segment.msg.data[6] = '-';
  offer->segment.msg.data[7] = ' ';
  Signal(TL_ReceivingQueueOffer, offer);
  
  //A simulated message from 111:2 to ???:0.
  //It's the final message fragment.
  offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offer->otherHostAddress = 111;
  offer->segment.is_first = 0;
  offer->segment.seqMsg = 1;
  offer->segment.seqPayload = 1;
  offer->segment.senderport = 2;
  offer->segment.receiverport = 0; //This one should correspond to the open connection 
  //offer->segment.msg.data = "TEST.\n";
  offer->segment.aux = 7;
  offer->segment.msg.data[0] = 'P';
  offer->segment.msg.data[1] = 'A';
  offer->segment.msg.data[2] = 'R';
  offer->segment.msg.data[3] = 'T';
  offer->segment.msg.data[4] = '2';
  offer->segment.msg.data[5] = '.';
  offer->segment.msg.data[6] = '\0';
  offer->segment.msg.data[7] = '\0';
  Signal(TL_ReceivingQueueOffer, offer);
  
  
  //logLine(succes, "     -----     : %s\n", &o.segment.msg.data);
  
  //Signal(TL_ReceivingQueueOffer, &offer);
  
  event_t event;
  long int events_we_handle = TL_ReceivingQueueOffer;// | TL_SocketRequest;
  while(true) {
    logLine(trace, "Waiting for signals.\n");
    Wait(&event, events_we_handle);
  
  
  o = *((TL_OfferElement*) event.msg);
  
  logLine(succes, "\nStarting Check.\n");
  
  //The port this segment is addressed to is actually valid.
  if(sockets[o.segment.receiverport] != NULL && sockets[o.segment.receiverport]->valid) {
    logLine(succes, "Socket %d is valid and is receiving a message.\n", o.segment.receiverport);
    
    //[PJ] A search is needed for this one because the data order is indeterministic.
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
      //If there is a hit, go for it. If not, then the message has to be a failure packet or a fraudulent packet.
      if(sockets[o.segment.receiverport]->connections[i].valid //Valid connection.
      && o.otherHostAddress == sockets[o.segment.receiverport]->connections[i].remoteAddress //The sender's address if the address of the connected machine
      && o.segment.senderport == sockets[o.segment.receiverport]->connections[i].remotePort //The sending application's port is the port of the connected application.
      ) {
        logLine(succes, "Connection at index %d matches the segment and is valid.\n", i);
        //The correct connection has been found. Handle the incoming payload.
        
        TLConnection *con = &(sockets[o.segment.receiverport]->connections[i]);
        
        logLine(succes, "Connection attributes: addr: %d, port: %d, head: %p, tail: %p\n", con->remoteAddress, con->remotePort, con->msgListHead, con->msgListTail);
        
        //New message start
        if(o.segment.is_first) {
          logLine(succes, "Message fragment is the first fragment.\n");
          
          //Make the new message to be inserted.
          TLMessageBufferLL *new = (TLMessageBufferLL*) malloc(sizeof(TLMessageBufferLL));
          new->next = NULL; //The pointers are defaulted to avoid having to remember to do this at all instances later.
          new->prev = NULL;
          new->msgLen = o.segment.aux;
          new->seqMsg = o.segment.seqMsg;
          new->fragmentsRemaining = o.segment.seqPayload; //See the .h file. Special case for the field is when is_first is enabled.
          
          //The message carried by this first fragment.
          new->msg = malloc(o.segment.aux); //Needs to allocate the message in order to memcpy into it.
          //The first might also be the last.
          int copyamount = MAX_PAYLOAD;
          if(new->msgLen < copyamount) {
            copyamount = new->msgLen;
          }
          logLine(succes, "The number of copied bytes: %d\n", copyamount);
          memcpy(new->msg, &(o.segment.msg), copyamount);
          logLine(succes, "Copied message: %s - %s\n", &(o.segment.msg), new->msg);
          
          
          //First case: there are no messages in the list.
          if(con->msgListHead == NULL) {
            con->msgListHead = new;
            con->msgListTail = con->msgListHead; //When there is a head, there should also be a tail. They just happen to be the same.
            logLine(succes, "Connection msg list head was NULL. Head: %p, Tail: %p\n", con->msgListHead, con->msgListTail);
            
          } else if(o.segment.seqMsg > con->msgListTail->seqMsg) {
            // ... -> Current Tail -> New Tail -> NULL
            TLMessageBufferLL *tmp = con->msgListTail;
            con->msgListTail = new;
            con->msgListTail->prev = tmp; //Old tail the is previous one from the new tail.
            tmp->next = new; //New tali is the next one from the old tail.
            logLine(succes, "Connection msg gets appended. Head: %p, Tail: %p\n", con->msgListHead, con->msgListTail);
            
          } else { //Gotta search for the correct spot. Going to do this from the tail because new messages will have numbers in the higher end most of the time.
            logLine(succes, "Connection msg goes somewhere inside the list. Message sequence numbers: new: %d, Tail: %d\n", o.segment.seqMsg, con->msgListTail->seqMsg);
            
            TLMessageBufferLL *i = con->msgListTail;
            while(i != NULL) {
              if(i->seqMsg < o.segment.seqMsg) {
                // ... -> i -> new -> i->next -> ...
                TLMessageBufferLL *inext = i->next;
                i->next = new; //I now has new as its next element.
                new->prev = i; // And I is before new.
                new->next = inext; //New now has i's old next as its next
                inext->prev = new; // And i's old next has new as its prev instead of i.
                
              }
              
              i = i->prev;
            }
            
          }
          
        } else { //Need to insert a fragment into a existing message.
          logLine(succes, "Message fragment is part of an existing message.\n");
          
          //Search from the front based on the assumption that the majority of fragments will be for the messages in the front of the list.
          //If excess time is available, having a pointer to a point in the list after the last complete message would be an optimization for when many messages queue up.
          for(TLMessageBufferLL *i = con->msgListHead; i != NULL && i->seqMsg <= o.segment.seqMsg; i = i->next) {
            if(i->seqMsg == o.segment.seqMsg) {
              //The element has been found.
              int copyamount = MAX_PAYLOAD;
              if((o.segment.seqPayload+1)*MAX_PAYLOAD >= i->msgLen) {
                copyamount = o.segment.aux;
              }
              
              memcpy((i->msg)+(o.segment.seqPayload*MAX_PAYLOAD), &(o.segment.msg), copyamount);
              i->fragmentsRemaining--;
              
              //Cut. Looping anymore would yield nothing.
              break;
            }
            
            
          }
          
          
          
        }
        
        
        logLine(succes, "\n\n");
        logLine(succes, "List contents:\n");
        for(TLMessageBufferLL *i = con->msgListHead; i != NULL; i = i->next) {
          if(i->fragmentsRemaining == 0) {
            logLine(succes, "%s\n", i->msg);
          }
        }
        
        
        
        break;
      }
    }
  }
  
  } //while loop
  
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

