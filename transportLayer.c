
#include "events.h"
#include "concurrentFIFOQueue.h"

#include "transportLayer.h"
#include "networkLayer.h"

void transportLayer() {
  FifoQueue sendingBuffQueue = InitializeFQ();
  ConcurrentFifoQueue sendingQueue = CFQ_Init();
  ConcurrentFifoQueue *offer;
  ConcurrentFifoQueue *q = malloc(sizeof(ConcurrentFifoQueue));
  *q = CFQ_Init();
  
  FifoQueueEntry e;
  TL_OfferElement o;
  TL_OfferElement *O;
  
  //TL_OfferElement *elem;
  
  #define TL_NUM_HEH 10
  /*int i = 1;
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
  */
  
  //----------------------------------------------------------------------------------------------------------------------------------------------------------
  
  #define NUM_MAX_SOCKETS 4
  #define SOCKET_ANY NUM_MAX_SOCKETS
  TLSocket *sockets[NUM_MAX_SOCKETS]; //the transPORT values correspond to indices in this array. SOCKET_ANY is any socket.
  
  for (int i = 0; i < NUM_MAX_SOCKETS; i++) {
    sockets[i] = NULL;
  }
  
  /*
  *   TEST CODE FOR RECEIVING FRAGMENTED MESSAGES.
  */
  /*
  //Register dummy variables.
  TL_OfferElement *offerelem;
  
  ConcurrentFifoQueue *testQueue = malloc(sizeof(ConcurrentFifoQueue));
  *testQueue = CFQ_Init();
  
  //EnqueueFQ(NewFQE((void*) offerelem), testQueue.queue);
  
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
  offerelem = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offerelem->otherHostAddress = 111;
  offerelem->segment.is_first = 1;
  offerelem->segment.seqMsg = 0;
  offerelem->segment.seqPayload = 0;
  offerelem->segment.senderport = 2;
  offerelem->segment.receiverport = 0; //This one should correspond to the open connection 
  //offerelem->segment.msg.data = "TEST.\n";
  offerelem->segment.aux = 7;
  offerelem->segment.msg.data[0] = 'T';
  offerelem->segment.msg.data[1] = 'E';
  offerelem->segment.msg.data[2] = 'S';
  offerelem->segment.msg.data[3] = 'T';
  offerelem->segment.msg.data[4] = '1';
  offerelem->segment.msg.data[5] = '.';
  offerelem->segment.msg.data[6] = '\0';
  offerelem->segment.msg.data[7] = '\0';
  //Signal(TL_ReceivingQueueOffer, offerelem);
  EnqueueFQ(NewFQE((void*) offerelem), testQueue->queue);
  
  //A simulated message from 111:2 to ???:0.
  //It's the first message with 1 more fragments.
  offerelem = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offerelem->otherHostAddress = 111;
  offerelem->segment.is_first = 1;
  offerelem->segment.seqMsg = 2;
  offerelem->segment.seqPayload = 0;
  offerelem->segment.senderport = 2;
  offerelem->segment.receiverport = 0; //This one should correspond to the open connection 
  //offerelem->segment.msg.data = "TEST.\n";
  offerelem->segment.aux = 7;
  offerelem->segment.msg.data[0] = 'T';
  offerelem->segment.msg.data[1] = 'E';
  offerelem->segment.msg.data[2] = 'S';
  offerelem->segment.msg.data[3] = 'T';
  offerelem->segment.msg.data[4] = '2';
  offerelem->segment.msg.data[5] = '.';
  offerelem->segment.msg.data[6] = '\0';
  offerelem->segment.msg.data[7] = '\0';
  //Signal(TL_ReceivingQueueOffer, offerelem);
  EnqueueFQ(NewFQE((void*) offerelem), testQueue->queue);
  
  //A simulated message from 111:2 to ???:0.
  //It's the first message with 1 more fragments.
  offerelem = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offerelem->otherHostAddress = 111;
  offerelem->segment.is_first = 1;
  offerelem->segment.seqMsg = 1;
  offerelem->segment.seqPayload = 1;
  offerelem->segment.senderport = 2;
  offerelem->segment.receiverport = 0; //This one should correspond to the open connection 
  //offerelem->segment.msg.data = "TEST.\n";
  offerelem->segment.aux = 8+7;
  offerelem->segment.msg.data[0] = 'P';
  offerelem->segment.msg.data[1] = 'A';
  offerelem->segment.msg.data[2] = 'R';
  offerelem->segment.msg.data[3] = 'T';
  offerelem->segment.msg.data[4] = '1';
  offerelem->segment.msg.data[5] = ' ';
  offerelem->segment.msg.data[6] = '-';
  offerelem->segment.msg.data[7] = ' ';
  //Signal(TL_ReceivingQueueOffer, offerelem);
  EnqueueFQ(NewFQE((void*) offerelem), testQueue->queue);
  
  //A simulated message from 111:2 to ???:0.
  //It's the final message fragment.
  offerelem = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offerelem->otherHostAddress = 111;
  offerelem->segment.is_first = 0;
  offerelem->segment.seqMsg = 1;
  offerelem->segment.seqPayload = 1;
  offerelem->segment.senderport = 2;
  offerelem->segment.receiverport = 0; //This one should correspond to the open connection 
  //offerelem->segment.msg.data = "TEST.\n";
  offerelem->segment.aux = 7;
  offerelem->segment.msg.data[0] = 'P';
  offerelem->segment.msg.data[1] = 'A';
  offerelem->segment.msg.data[2] = 'R';
  offerelem->segment.msg.data[3] = 'T';
  offerelem->segment.msg.data[4] = '2';
  offerelem->segment.msg.data[5] = '.';
  offerelem->segment.msg.data[6] = '\0';
  offerelem->segment.msg.data[7] = '\0';
  //Signal(TL_ReceivingQueueOffer, offerelem);
  EnqueueFQ(NewFQE((void*) offerelem), testQueue->queue);
  
  TL_OfferReceivingQueue(testQueue);
  */
  
  
  
  TLSocket** socket;
  TLSockReq* req;
  
  //int numReceivedPackets = 0;
  
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
          
          //The first debugging line is no longer valid. Messages now come in the segment field instead of the seg field.
          //logLine(succes, "TL: Received from host with address %d: '%s'\n", o.otherHostAddress, o.seg.data);
          //logLine(succes, "\n                    TL: Received from host with address %d: '%s'\n\n", o.otherHostAddress, o.seg.data);
          
          
          // Socket Part
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
            
            //Check first if the element goes in front. An easy, constant time solution to prevent complexity arising in the loop below.
            if(o.segment.seqMsg < con->msgListHead->seqMsg) {
              new->next = con->msgListHead; //The new head has the old head as its next.
              con->msgListHead->prev = new; //The old head has the new head as its previous.
              con->msgListHead = new; //Assign the new head.
            } else {
              
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
          
          // Endof: Socket Part
          
        }
        
        //logLine(trace, "TL: Releasing locks.\n");
        //Unlock(offer->lock);
        //sleep(2); //TEMPORARY!!!
        //Stop();
        
        //numReceivedPackets++;
        
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
        
        //Default values that would otherwise contain garbage.
        for(int i = 0; i < MAX_CONNECTIONS; i++) {
          sockets[req->port]->connections[i].valid = 0;
          sockets[req->port]->connections[i].remoteAddress = 0;
          sockets[req->port]->connections[i].remotePort = 0;
          sockets[req->port]->connections[i].outboundSeqMsg = 0;
          sockets[req->port]->connections[i].msgListHead = NULL;
          sockets[req->port]->connections[i].msgListTail = NULL;
        }
        
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
      
      case AL_Send: //Split the message and send the fragments as segments.
      ;
      ALMessageSend *MS = (ALMessageSend*) event.msg;
      
      TLSocket *socketToUse = MS->socketToUse;
      unsigned int connectionToUse = MS->connectionid;
      
  //TLSocket *socketToUse = sockets[1];
  //unsigned int connectionToUse = 0;
  networkAddress targetAddress = socketToUse->connections[connectionToUse].remoteAddress;
  transPORT targetPort = socketToUse->connections[connectionToUse].remotePort;
  unsigned int seqMsg = socketToUse->connections[connectionToUse].outboundSeqMsg;
  
  char* msgToSplit = "SPLIT ME PLEASE!!!!"; //"12345678abcdefghYNYNYNYN"; //"ASDFasdf"; //"ASDF\n";
  unsigned int msglen = 20;
  int cpa = MAX_PAYLOAD;
  
  int numFragments = msglen / MAX_PAYLOAD;
  if(numFragments*MAX_PAYLOAD < msglen) {numFragments++;};
  
  if(msglen < cpa) {cpa = msglen;}
  logLine(succes, "Splitting message %s with msglen %d, cpa %d, numFragments: %d\n", msgToSplit, msglen, cpa, numFragments);
  
  //First fragment. Set to be the fourth message (3).
  O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  O->otherHostAddress = targetAddress; //111;
  O->segment.is_first = 1;
  O->segment.seqMsg = seqMsg; //4;
  O->segment.seqPayload = numFragments-1;
  O->segment.senderport = socketToUse->port; //2;
  O->segment.receiverport = targetPort; //0; //This one should correspond to the open connection 
  //O->segment.msg.data = "TEST.\n";
  O->segment.aux = msglen;
  memcpy(&(O->segment.msg.data), msgToSplit, cpa);
  /*O->segment.msg.data[0] = 'P';
  O->segment.msg.data[1] = 'A';
  O->segment.msg.data[2] = 'R';
  O->segment.msg.data[3] = 'T';
  O->segment.msg.data[4] = '1';
  O->segment.msg.data[5] = ' ';
  O->segment.msg.data[6] = '-';
  O->segment.msg.data[7] = ' ';*/
  //Signal(TL_ReceivingQueueOffer, O);
  
  int i = 0;
  logLine(succes, "   %p, %p\n", &(O->segment.msg)+(i), msgToSplit+(i*MAX_PAYLOAD));
  while(true) {
  //for(int i = MAX_PAYLOAD; i < msglen; i += MAX_PAYLOAD) {
    //Submit the previous one.
    //Signal(TL_ReceivingQueueOffer, O); //Is used in the testing version.
    EnqueueFQ( NewFQE( (void *) O ), sendingBuffQueue);
    
    i++; //Important that this comes first.
    
    if(i >= numFragments) {
      break;
    }
    
    logLine(succes, "Building additional fragment with seqPayload: %d\n", i);
    
    O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
    O->otherHostAddress = targetAddress; //111;
    O->segment.is_first = 0;
    O->segment.seqMsg = seqMsg; //4;
    O->segment.seqPayload = i;
    O->segment.senderport = socketToUse->port; //2;
    O->segment.receiverport = targetPort; //0; //This one should correspond to the open connection 
    //O->segment.msg.data = "TEST.\n";
    O->segment.aux = 0;
    
              /*int copyamount = MAX_PAYLOAD;
              if((o.segment.seqPayload+1)*MAX_PAYLOAD >= i->msgLen) {
                copyamount = o.segment.aux;
              }
              
              memcpy((i->msg)+(o.segment.seqPayload*MAX_PAYLOAD), &(o.segment.msg), copyamount);*/
    
    cpa = MAX_PAYLOAD;
    //Is this the last fragment?
    if((i+1)*MAX_PAYLOAD > msglen) {
      O->segment.aux = msglen % MAX_PAYLOAD;
      cpa = msglen % MAX_PAYLOAD;
    }
    
  logLine(succes, "   %p, %p\n", &(O->segment.msg)+(i*MAX_PAYLOAD), msgToSplit+(i*MAX_PAYLOAD));
    memcpy(&(O->segment.msg.data), msgToSplit+(i*MAX_PAYLOAD), MAX_PAYLOAD);
    /*O->segment.msg.data[0] = 'P';
    O->segment.msg.data[1] = 'A';
    O->segment.msg.data[2] = 'R';
    O->segment.msg.data[3] = 'T';
    O->segment.msg.data[4] = '1';
    O->segment.msg.data[5] = ' ';
    O->segment.msg.data[6] = '-';
    O->segment.msg.data[7] = ' ';*/
    //Signal(TL_ReceivingQueueOffer, O);
    
    logLine(succes, "frag bilt wit: %d, %d, %d\n", O->segment.seqMsg, O->segment.seqPayload, cpa);
    
  }
  
  //The next msg seq nr should be 1 larger.
  socketToUse->connections[connectionToUse].outboundSeqMsg += 1;
        
        
        
        
        
        break;
    }
    
    //If the lock is free, then put the received elements into the queue for the LL.
    if(sendingQueue.used == false && EmptyFQ(sendingBuffQueue) == 0) {
      logLine(trace, "TL: Transfering between sending queues\n");
      //Lock(sendingQueue.lock); //Nothing else uses it, but for good measure since it's easy here.
      
      //Transfer from one queue to the other.
      while(EmptyFQ(sendingBuffQueue) == 0) {
        EnqueueFQ(DequeueFQ(sendingBuffQueue), sendingQueue.queue);
      }
      
      //Unlock(sendingQueue.lock);
    }
    
    
    //[PJ] DEPRECATED WITH INTRODUCTION OF APPLICATION LAYER. DELETE WHEN APPROPRIATE.
    //if(numReceivedPackets >= TL_NUM_HEH) {
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
      /*
      sleep(2);
      Stop();
    }*/
    
    
  }
  
}

void socketCodeTest() {
  
  TL_OfferElement *offer;
  TL_OfferElement o;
  
  TLSocket *sockets[NUM_MAX_SOCKETS]; //the transPORT values correspond to indices in this array. SOCKET_ANY is any socket.
  
  for (int i = 0; i < NUM_MAX_SOCKETS; i++) {
    sockets[i] = NULL;
  }
  //Socket opened on port 0 with connection to 111:2
  sockets[0] = malloc(sizeof(TLSocket));
  sockets[0]->valid = 1;
  sockets[0]->listening = 0;
  sockets[0]->port = 0; //Because the hypothetical AL can read this field to determine which port it has gotten.
  sockets[0]->connections[0].valid = 1;
  sockets[0]->connections[0].remoteAddress = 111;
  sockets[0]->connections[0].remotePort = 1;
  sockets[0]->connections[0].msgListHead = NULL;
  sockets[0]->connections[0].msgListTail = NULL;
  
  //Socket opened on port 1 with connection to 111:0 //Pretending that 111 is itself.
  sockets[1] = malloc(sizeof(TLSocket));
  sockets[1]->valid = 1;
  sockets[1]->listening = 0;
  sockets[1]->port = 1; //Because the hypothetical AL can read this field to determine which port it has gotten.
  sockets[1]->connections[0].valid = 1;
  sockets[1]->connections[0].remoteAddress = 111;
  sockets[1]->connections[0].remotePort = 0;
  sockets[1]->connections[0].outboundSeqMsg = 4;
  sockets[1]->connections[0].msgListHead = NULL;
  sockets[1]->connections[0].msgListTail = NULL;
  
  
  // MESSAGE SPLITTING
  
  TLSocket *socketToUse = sockets[1];
  unsigned int connectionToUse = 0;
  networkAddress targetAddress = socketToUse->connections[connectionToUse].remoteAddress;
  transPORT targetPort = socketToUse->connections[connectionToUse].remotePort;
  unsigned int seqMsg = socketToUse->connections[connectionToUse].outboundSeqMsg;
  
  char* msgToSplit = "SPLIT ME PLEASE!!!!"; //"12345678abcdefghYNYNYNYN"; //"ASDFasdf"; //"ASDF\n";
  unsigned int msglen = 20;
  int cpa = MAX_PAYLOAD;
  
  int numFragments = msglen / MAX_PAYLOAD;
  if(numFragments*MAX_PAYLOAD < msglen) {numFragments++;};
  
  if(msglen < cpa) {cpa = msglen;}
  logLine(succes, "Splitting message %s with msglen %d, cpa %d, numFragments: %d\n", msgToSplit, msglen, cpa, numFragments);
  
  //First fragment. Set to be the fourth message (3).
  offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offer->otherHostAddress = targetAddress; //111;
  offer->segment.is_first = 1;
  offer->segment.seqMsg = seqMsg; //4;
  offer->segment.seqPayload = numFragments-1; //TODO Important: calculate this!
  offer->segment.senderport = socketToUse->port; //2;
  offer->segment.receiverport = targetPort; //0; //This one should correspond to the open connection 
  //offer->segment.msg.data = "TEST.\n";
  offer->segment.aux = msglen;
  memcpy(&(offer->segment.msg.data), msgToSplit, cpa);
  /*offer->segment.msg.data[0] = 'P';
  offer->segment.msg.data[1] = 'A';
  offer->segment.msg.data[2] = 'R';
  offer->segment.msg.data[3] = 'T';
  offer->segment.msg.data[4] = '1';
  offer->segment.msg.data[5] = ' ';
  offer->segment.msg.data[6] = '-';
  offer->segment.msg.data[7] = ' ';*/
  //Signal(TL_ReceivingQueueOffer, offer);
  
  int i = 0;
  logLine(succes, "   %p, %p\n", &(offer->segment.msg)+(i), msgToSplit+(i*MAX_PAYLOAD));
  while(true) {
  //for(int i = MAX_PAYLOAD; i < msglen; i += MAX_PAYLOAD) {
    //Submit the previous one.
    Signal(TL_ReceivingQueueOffer, offer);
    
    i++; //Important that this comes first.
    
    if(i >= numFragments) {
      break;
    }
    
    logLine(succes, "Building additional fragment with seqPayload: %d\n", i);
    
    offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
    offer->otherHostAddress = targetAddress; //111;
    offer->segment.is_first = 0;
    offer->segment.seqMsg = seqMsg; //4;
    offer->segment.seqPayload = i; //TODO Important: calculate this!
    offer->segment.senderport = socketToUse->port; //2;
    offer->segment.receiverport = targetPort; //0; //This one should correspond to the open connection 
    //offer->segment.msg.data = "TEST.\n";
    offer->segment.aux = 0;
    
              /*int copyamount = MAX_PAYLOAD;
              if((o.segment.seqPayload+1)*MAX_PAYLOAD >= i->msgLen) {
                copyamount = o.segment.aux;
              }
              
              memcpy((i->msg)+(o.segment.seqPayload*MAX_PAYLOAD), &(o.segment.msg), copyamount);*/
    
    cpa = MAX_PAYLOAD;
    //Is this the last fragment?
    if((i+1)*MAX_PAYLOAD > msglen) {
      offer->segment.aux = msglen % MAX_PAYLOAD;
      cpa = msglen % MAX_PAYLOAD;
    }
    
  logLine(succes, "   %p, %p\n", &(offer->segment.msg)+(i*MAX_PAYLOAD), msgToSplit+(i*MAX_PAYLOAD));
    memcpy(&(offer->segment.msg.data), msgToSplit+(i*MAX_PAYLOAD), MAX_PAYLOAD);
    /*offer->segment.msg.data[0] = 'P';
    offer->segment.msg.data[1] = 'A';
    offer->segment.msg.data[2] = 'R';
    offer->segment.msg.data[3] = 'T';
    offer->segment.msg.data[4] = '1';
    offer->segment.msg.data[5] = ' ';
    offer->segment.msg.data[6] = '-';
    offer->segment.msg.data[7] = ' ';*/
    //Signal(TL_ReceivingQueueOffer, offer);
    
    logLine(succes, "frag bilt wit: %d, %d, %d\n", offer->segment.seqMsg, offer->segment.seqPayload, cpa);
    
  }
  
  //The next msg seq nr should be 1 larger.
  socketToUse->connections[connectionToUse].outboundSeqMsg += 1;
  
  
  
  //PREDETERMINED
  
  //A simulated message from 111:2 to ???:0.
  //It's the first message with 0 more fragments.
  offer = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  offer->otherHostAddress = 111;
  offer->segment.is_first = 1;
  offer->segment.seqMsg = 0;
  offer->segment.seqPayload = 0;
  offer->segment.senderport = 1;
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
  offer->segment.senderport = 1;
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
  offer->segment.senderport = 1;
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
  offer->segment.senderport = 1;
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
  
  logLine(succes, "Incoming message for socket at port %d\n", o.segment.receiverport);
  
  //The port this segment is addressed to is actually valid.
  if(sockets[o.segment.receiverport] != NULL && sockets[o.segment.receiverport]->valid) {
    logLine(succes, "Socket %d is valid and is receiving a message.\n", o.segment.receiverport);
    logLine(succes, "Message addressed for: address %d and port %d\n", o.otherHostAddress, o.segment.receiverport);
    
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
            
            //Check first if the element goes in front. An easy, constant time solution to prevent complexity arising in the loop below.
            if(o.segment.seqMsg < con->msgListHead->seqMsg) {
              new->next = con->msgListHead; //The new head has the old head as its next.
              con->msgListHead->prev = new; //The old head has the new head as its previous.
              con->msgListHead = new; //Assign the new head.
            } else {
            
            TLMessageBufferLL *i = con->msgListTail;
            logLine(succes, "   i/Tail: %p\n", i);
            while(i != NULL) {
              logLine(succes, "   Loop: i/Tail: %p, i->seqMsg: %d, o.segment.seqMsg: %d\n", i, i->seqMsg, o.segment.seqMsg);
              if(i->seqMsg < o.segment.seqMsg) {
                // ... -> i -> new -> i->next -> ...
                TLMessageBufferLL *inext = i->next;
            //logLine(succes, "   Loop vals before: inext: %p, new: %p, new->prev %p, new->next: %p, inext->prev: %p\n", inext, new, new->prev, new->next, inext->prev);
                
                i->next = new; //I now has new as its next element.
                new->prev = i; // And I is before new.
                new->next = inext; //New now has i's old next as its next
                inext->prev = new; // And i's old next has new as its prev instead of i.
            //logLine(succes, "   Loop vals after: inext: %p, new: %p, new->prev %p, new->next: %p, inext->prev: %p\n", inext, new, new->prev, new->next, inext->prev);
                
                logLine(succes, "   Moved one forward in the list.");
              }
              
              i = i->prev;
            }
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
              
              logLine(succes, "Message fragment is copied into message at address: %p, with %d bytes copied. %d fragments remain. The sequence number of the fragment was: %d\n", i, copyamount, i->fragmentsRemaining, o.segment.seqPayload);
              
              logLine(succes, "%c%c%c%c%c%c%c%c\n", o.segment.msg.data[0], o.segment.msg.data[1], o.segment.msg.data[2], o.segment.msg.data[3], o.segment.msg.data[4], o.segment.msg.data[5], o.segment.msg.data[6], o.segment.msg.data[7]);
              
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

