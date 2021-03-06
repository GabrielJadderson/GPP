
#include "events.h"
#include "concurrentFIFOQueue.h"

//#include "transportLayer.h"
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

  int conid = 0;


  TLSocket** socket;
  TLSockReq* req;

  //int numReceivedPackets = 0;

  event_t event;
  long int events_we_handle = TL_ReceivingQueueOffer | TL_SocketRequest | AL_Send | AL_Receive | AL_Disconnect | AL_Connect;
  while(true) {
    logLine(trace, "TL: Waiting for signals.\n");
    //logLine(succes, "TL: Lockstatus=%d\n", q->used);
    Wait(&event, events_we_handle);

    switch(event.type) {
      case AL_Connect:
        logLine(debug, "AL: Received signal AL_Connect\n");
        ALConnReq *connReq = (ALConnReq*) event.msg;

        boolean gotAssigned = false;
        for (int i = 0; i < MAX_CONNECTIONS && gotAssigned == false; i++) {
          if (connReq->sock->connections[i].valid == 0 && connReq->sock->connections[i].pending == 0 && connReq->sock->connections[i].disconnected == 0) { //check for unused connection
              connReq->sock->connections[i].pending = 1; //assign that this connection is pending for a response.
              connReq->sock->connections[i].disconnected = 0; //assign that this connection is pending for a response.
              connReq->sock->connections[i].remoteAddress = connReq->netAddress;
              //Reset these values.
              connReq->sock->connections[conid].outboundSeqMsg = 0;
              connReq->sock->connections[conid].inboundSeqMsg = 0;
              connReq->sock->connections[conid].msgListHead = NULL;
              connReq->sock->connections[conid].msgListTail = NULL;

              connReq->connectionid = i; //update the connReq to return in AL
              gotAssigned = true; //indicate that the connection was assigned sucessfully.
              conid = i;
              break;
          }
        }
        if (gotAssigned == false) {
          logLine(error, "AL_Connect: Failed to assign connection NO SPACE/AVAILABLE CONNECTIONS\n");
          connReq->connectionid = CONNECTION_FAILURE; //update the connReq to return in AL
          break;
        }
        logLine(succes, "ESTABLISHING CONNECTION AT CONNECTION %d ON PORT %d\n", conid, connReq->sock->port);


        O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
        O->otherHostAddress = connReq->netAddress;
        O->segment.is_first = 1;
        O->segment.is_control = 1;
        O->segment.seqMsg = 0;
        O->segment.seqPayload = 0;
        O->segment.senderport = connReq->sock->port;
        O->segment.receiverport = connReq->port; //This one should correspond to the open connection
        O->segment.aux = 0;
        memset(&(O->segment.msg.data), 0, 8);

        EnqueueFQ( NewFQE( (void *) O ), sendingBuffQueue);
        logLine(debug, "AL: SENDING TO THE BUFFER QUEUE\n");
        break;

      case AL_Disconnect:
        logLine(debug, "AL: Received signal AL_Disconnect\n");

        ALDisconnectReq* disconnectReq = (ALDisconnectReq*) event.msg;

        //If the other end has disconnected, just clean up our own side.
        if(disconnectReq->sock->connections[disconnectReq->connectionid].disconnected == 0) {
          O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
          O->otherHostAddress = disconnectReq->sock->connections[disconnectReq->connectionid].remoteAddress;
          O->segment.is_first = 1;
          O->segment.is_control = 1;
          O->segment.seqMsg = 0;
          O->segment.seqPayload = 0;
          O->segment.senderport = disconnectReq->sock->port;
          O->segment.receiverport = disconnectReq->sock->connections[disconnectReq->connectionid].remotePort; //This one should correspond to the open connection
          O->segment.aux = 1;
          memset(&(O->segment.msg.data), 0, 8);

          EnqueueFQ( NewFQE( (void *) O ), sendingBuffQueue);
        }

        //The disconnecting side assumes that all the important messages have been read.
        disconnectReq->sock->connections[disconnectReq->connectionid].valid = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].disconnected = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].pending = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].remoteAddress = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].remotePort = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].outboundSeqMsg = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].inboundSeqMsg = 0;
        disconnectReq->sock->connections[disconnectReq->connectionid].msgListHead = NULL; //Leaking everything. This should ideally be cleaned up.
        disconnectReq->sock->connections[disconnectReq->connectionid].msgListTail = NULL;

        logLine(debug, "AL: DISCONNECTED CONNECTION ID \n");
        break;
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
  logLine(trace, "\nStarting Check.\n");
  logLine(debug, "LALALALALALA *******: receiverport: %d, socket@rcv: %p, is_first: %d, is_control: %d, seqMsg: %d, seqPayload: %d, aux: %d\n", o.segment.receiverport, sockets[o.segment.receiverport], o.segment.is_first, o.segment.is_control, o.segment.seqMsg, o.segment.seqPayload, o.segment.aux);

  //The port this segment is addressed to is actually valid.
  if(sockets[o.segment.receiverport] != NULL && sockets[o.segment.receiverport]->valid) {
    logLine(debug, "Socket %d is valid and is receiving a message.\n", o.segment.receiverport);

    //If the segment is a control segment, then this is where to deal with it.
    if(o.segment.is_control) {
      logLine(trace, "Handling control segment.\n");

      if(o.segment.aux == 0) { //Connect.
        if(o.segment.is_first) { //Connection request received.

          //See if there is a slot available.
          for(conid = 0; conid < MAX_CONNECTIONS; conid++) {
            if(sockets[o.segment.receiverport]->connections[conid].valid == 0
            && sockets[o.segment.receiverport]->connections[conid].pending == 0
            && sockets[o.segment.receiverport]->connections[conid].disconnected == 0) {
              break;
            }
          }

          logLine(succes, "INCOMING CONNECTION AT CONNECTION %d ON PORT %d\n", conid, o.segment.receiverport);

          //No room. Send aux=2, connection refused.
          if(conid >= MAX_CONNECTIONS) {
            O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
            O->otherHostAddress = o.otherHostAddress;
            O->segment.is_first = 0;
            O->segment.is_control = 1;
            O->segment.seqMsg = 0;
            O->segment.seqPayload = 0;
            O->segment.senderport = sockets[o.segment.receiverport]->port;
            O->segment.receiverport = o.segment.senderport; //This one should correspond to the open connection
            O->segment.aux = 2;
            memset(&(O->segment.msg.data), 0, 8);
            break; //Failure.
          }

          //Otherwise, configure the connection slot.
          sockets[o.segment.receiverport]->connections[conid].valid = 1;
          sockets[o.segment.receiverport]->connections[conid].disconnected = 0;
          sockets[o.segment.receiverport]->connections[conid].pending = 0;
          sockets[o.segment.receiverport]->connections[conid].remoteAddress = o.otherHostAddress;
          sockets[o.segment.receiverport]->connections[conid].remotePort = o.segment.senderport;
          sockets[o.segment.receiverport]->connections[conid].outboundSeqMsg = 0;
          sockets[o.segment.receiverport]->connections[conid].inboundSeqMsg = 0;
          sockets[o.segment.receiverport]->connections[conid].msgListHead = NULL;
          sockets[o.segment.receiverport]->connections[conid].msgListTail = NULL;

          //If the socket was set to listen, mark is as not listening, effectively hosting a connection.
          logLine(succes, "Listening status: %d, con being: %d\n", sockets[o.segment.receiverport]->listening, sockets[o.segment.receiverport]->listenConnection);
          sockets[o.segment.receiverport]->listening |= 2;
          sockets[o.segment.receiverport]->listenConnection = conid; //This is the id of the connection that has been estalished.

          //Send verification response, aux=0, is_first=0. "Connection query that isn't initiating"
          O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
          O->otherHostAddress = o.otherHostAddress;
          O->segment.is_first = 0;
          O->segment.is_control = 1;
          O->segment.seqMsg = 0;
          O->segment.seqPayload = 0;
          O->segment.senderport = sockets[o.segment.receiverport]->port;
          O->segment.receiverport = o.segment.senderport; //This one should correspond to the open connection
          O->segment.aux = 0;
          memset(&(O->segment.msg.data), 0, 8);

          EnqueueFQ( NewFQE( (void *) O ), sendingBuffQueue);

         } else { //Connection request verification response received
          logLine(succes, "CONNECTION ACCEPTED!\n");
          //Search for the connection that gets the response.
          for(conid = 0; conid < MAX_CONNECTIONS; conid++) {
            if(sockets[o.segment.receiverport]->connections[conid].pending == 1 //The connection is actually pending such a reply.
               //Actual search criteria.
            && sockets[o.segment.receiverport]->connections[conid].remoteAddress == o.otherHostAddress
            && sockets[o.segment.receiverport]->connections[conid].remotePort == o.segment.senderport
              ) {
              //A pending connection has been given green light. Time to make it valid.
              sockets[o.segment.receiverport]->connections[conid].valid = 1;
              sockets[o.segment.receiverport]->connections[conid].pending = 0; //Got reply. No longer pending.
              //The other values have been set in the AL_Connect signal handling.
              break;
            }
          }
        }
      } else if(o.segment.aux == 1) { //Disconnect
          //Search for the connection to disconnect.
          for(conid = 0; conid < MAX_CONNECTIONS; conid++) {
            if(sockets[o.segment.receiverport]->connections[conid].valid == 1 //The connection to disconnect is actually valid.
               //Actual search criteria.
            && sockets[o.segment.receiverport]->connections[conid].remoteAddress == o.otherHostAddress
            && sockets[o.segment.receiverport]->connections[conid].remotePort == o.segment.senderport
              ) {
              //A pending connection has been given green light. Time to make it valid.
              sockets[o.segment.receiverport]->connections[conid].disconnected = 1;
              sockets[o.segment.receiverport]->connections[conid].pending = 0; //Got reply. No longer pending.
              sockets[o.segment.receiverport]->connections[conid].remoteAddress = 0;
              sockets[o.segment.receiverport]->connections[conid].remotePort = 0;
              sockets[o.segment.receiverport]->connections[conid].outboundSeqMsg = 0;
              sockets[o.segment.receiverport]->connections[conid].inboundSeqMsg = 0;
              break;
            }
          }

      } else if(o.segment.aux == 2) { //Connection refused.
        //Search for the connection that gets the response.
        logLine(succes, "CONNECTION REFUSED!\n");
        for(conid = 0; conid < MAX_CONNECTIONS; conid++) {
          if(sockets[o.segment.receiverport]->connections[conid].pending == 1 //The connection is actually pending such a reply.
             //Actual search criteria.
          && sockets[o.segment.receiverport]->connections[conid].remoteAddress == o.otherHostAddress
          && sockets[o.segment.receiverport]->connections[conid].remotePort == o.segment.senderport
            ) {
            //A pending connection has been given green light. Time to make it valid.
            sockets[o.segment.receiverport]->connections[conid].valid = 0;
            sockets[o.segment.receiverport]->connections[conid].pending = 0; //Got reply. No longer pending.
            //The other values have been set in the AL_Connect signal handling.
            break;
          }
        }
      }

      continue; //Done with this segment. Go to the next one.
    }

    //[PJ] A search is needed for this one because the data order is indeterministic.
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
      //If there is a hit, go for it. If not, then the message has to be a failure packet or a fraudulent packet.
      if(sockets[o.segment.receiverport]->connections[i].valid //Valid connection.
      && o.otherHostAddress == sockets[o.segment.receiverport]->connections[i].remoteAddress //The sender's address if the address of the connected machine
      && o.segment.senderport == sockets[o.segment.receiverport]->connections[i].remotePort //The sending application's port is the port of the connected application.
      ) {
        logLine(debug, "Connection at index %d matches the segment and is valid.\n", i);
        //The correct connection has been found. Handle the incoming payload.

        TLConnection *con = &(sockets[o.segment.receiverport]->connections[i]);

        logLine(debug, "Connection attributes: addr: %d, port: %d, head: %p, tail: %p\n", con->remoteAddress, con->remotePort, con->msgListHead, con->msgListTail);

        //New message start
        if(o.segment.is_first) {
          logLine(debug, "Message fragment is the first fragment.\n");

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
          logLine(trace, "The number of copied bytes: %d\n", copyamount);
          memcpy(new->msg, &(o.segment.msg), copyamount);
          logLine(debug, "Copied message: %s - %s\n", &(o.segment.msg), new->msg);


          //First case: there are no messages in the list.
          if(con->msgListHead == NULL) {
            con->msgListHead = new;
            con->msgListTail = con->msgListHead; //When there is a head, there should also be a tail. They just happen to be the same.
            logLine(debug, "Connection msg list head was NULL. Head: %p, Tail: %p\n", con->msgListHead, con->msgListTail);

          } else if(o.segment.seqMsg > con->msgListTail->seqMsg) {
            // ... -> Current Tail -> New Tail -> NULL
            TLMessageBufferLL *tmp = con->msgListTail;
            con->msgListTail = new;
            con->msgListTail->prev = tmp; //Old tail the is previous one from the new tail.
            tmp->next = new; //New tali is the next one from the old tail.
            logLine(debug, "Connection msg gets appended. Head: %p, Tail: %p\n", con->msgListHead, con->msgListTail);

          } else { //Gotta search for the correct spot. Going to do this from the tail because new messages will have numbers in the higher end most of the time.
            logLine(debug, "Connection msg goes somewhere inside the list. Message sequence numbers: new: %d, Tail: %d\n", o.segment.seqMsg, con->msgListTail->seqMsg);

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
          logLine(debug, "Message fragment is part of an existing message.\n");

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

        /*
        logLine(info, "\n\n");
        logLine(info, "List contents:\n");
        for(TLMessageBufferLL *i = con->msgListHead; i != NULL; i = i->next) {
          if(i->fragmentsRemaining == 0) {
            logLine(debug, "Msg@%p has msg* %p with string: %s\n", i, i->msg, i->msg);
            logLine(succes, "%s\n", i->msg); //[PJ] Succes level until the receive function is in place.
            //logLine(succes, "%s\n", (i->msg)+8);
            //logLine(succes, "%s\n", (i->msg)+16);
            int asdf = 0; //[PJ] WARNING: THIS LOOP DOESN'T WORK!
            for(int j = 0; j < 20; j++) {
              if(i->msg+j != 0) {
                asdf = 1;
                logLine(succes, "Byte %d: %c\n", j, i->msg+j);
              }
            }

            if(asdf) {
              logLine(succes, "Not all bytes 0!\n");
            } else {
              logLine(succes, "All bytes 0!\n");
            }
          }
        }*/



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
        req = (TLSockReq*) event.msg;
        logLine(info, "TL: Socket Requested with port %d\n", req->port);

        //TODO: [PJ] Should first check if the port is available.

        if(req->port == SOCKET_ANY) {
          logLine(info, "Request to any port.\n");
          for(int i = 0; i < NUM_MAX_SOCKETS; i++) {
            if(sockets[i] == NULL) {
              req->port = i;
              logLine(info, "Decided on port number %d\n", req->port);
              break;
            }
          }

         if(req->port == SOCKET_ANY) { //Copy-paste, WEE!
            logLine(info, "Unable to deliver requested socket.\n");
            socket = malloc(sizeof(TLSocket*));
            (*socket) = malloc(sizeof(TLSocket));
            (*socket)->valid = 0;
            req->sock = (*socket);
            socket = NULL; //For good measure.
            break;
          }

        } else {
          if(sockets[req->port] != NULL || req->port >= NUM_MAX_SOCKETS) { // [PJ] Port taken/port invalid. Return an invalid port. Everything else than the valid field can be anything.
            logLine(info, "Unable to deliver requested socket.\n");
            socket = malloc(sizeof(TLSocket*));
            (*socket) = malloc(sizeof(TLSocket));
            (*socket)->valid = 0;
            req->sock = (*socket);
            socket = NULL; //For good measure.
            break;
          }
        }

        logLine(trace, "Allocating socket.\n");
        sockets[req->port] = malloc(sizeof(TLSocket)); //[PJ] Fixed: This was mallocing the size of a pointer, causing an overwrite of a linked list header in the malloc implementation.

        logLine(trace, "Setting socket port and validity.\n");
        (sockets[req->port])->port = req->port; //Because the information being shared with the application.
        (sockets[req->port])->valid = 1; //The port is valid.
        (sockets[req->port])->listening = 0; //The port is valid.
        (sockets[req->port])->listenConnection = CONNECTION_PENDING; //The port is valid.

        logLine(trace, "Setting conection default values.\n");
        //Default values that would otherwise contain garbage.
        for(int i = 0; i < MAX_CONNECTIONS; i++) {
          logLine(trace, "Setting for %d\n", i);
          sockets[req->port]->connections[i].valid = 0;
          sockets[req->port]->connections[i].pending = 0;
          sockets[req->port]->connections[i].disconnected = 0;
          sockets[req->port]->connections[i].remoteAddress = 0;
          sockets[req->port]->connections[i].remotePort = 0;
          sockets[req->port]->connections[i].outboundSeqMsg = 0;
          sockets[req->port]->connections[i].inboundSeqMsg = 0;
          sockets[req->port]->connections[i].msgListHead = NULL;
          sockets[req->port]->connections[i].msgListTail = NULL;
          logLine(trace, "Done setting for %d\n", i);
        }

        logLine(trace, "Setting request socket pointer to resulting socket.");
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

        logLine(trace, "Socket delivered. Breaking.");
        break;

      case AL_Receive:
        logLine(trace, "AL_RECEIVE!\n");

        ALMessageSend *MR = (ALMessageSend*) event.msg;

        TLSocket *socket = MR->socketToUse;

        //logLine(succes, "%d %p %d %d %d\n", socket->connections[MR->connectionid].valid, socket->connections[MR->connectionid].msgListHead != NULL, socket->connections[MR->connectionid].inboundSeqMsg, (socket->connections[MR->connectionid].msgListHead)->seqMsg, (socket->connections[MR->connectionid].msgListHead)->fragmentsRemaining);
        logLine(debug, "%d %p \n", socket->connections[MR->connectionid].valid, socket->connections[MR->connectionid].msgListHead);
        if(socket->connections[MR->connectionid].msgListHead != NULL){
          logLine(debug, "sequence ids: %d, %d, %d\n", socket->connections[MR->connectionid].inboundSeqMsg, (socket->connections[MR->connectionid].msgListHead)->seqMsg, (socket->connections[MR->connectionid].msgListHead)->fragmentsRemaining);
        }

        if(socket->valid //Socket is valid
          && socket->connections[MR->connectionid].valid //Connection is valid
          && socket->connections[MR->connectionid].msgListHead != NULL //There is a message
          && socket->connections[MR->connectionid].inboundSeqMsg == (socket->connections[MR->connectionid].msgListHead)->seqMsg //The message is the one we want
          && (socket->connections[MR->connectionid].msgListHead)->fragmentsRemaining == 0 //The whole message has arrived.
          ) {
          //Time to give the inbound message to the application.
          TLMessageBufferLL *tmp = socket->connections[MR->connectionid].msgListHead;
          MR->length = tmp->msgLen;
          MR->message = tmp->msg;

          socket->connections[MR->connectionid].msgListHead = tmp->next;
          free(tmp);

          socket->connections[MR->connectionid].inboundSeqMsg++;
        }

        MR->aux = 0;
        //Intentionally leaving message as NULL if the conditions aren't true.
        break;

      case AL_Send: //Split the message and send the fragments as segments.
      ;
      ALMessageSend *MS = (ALMessageSend*) event.msg;
      TLSocket *socketToUse = MS->socketToUse;
      unsigned int connectionToUse = MS->connectionid;

      if(socketToUse->valid == 0 || socketToUse->connections[connectionToUse].valid == 0) {
        break;
      }

      char* msgToSplit = MS->message;
      unsigned int msglen = MS->length;


  //TLSocket *socketToUse = sockets[1];
  //unsigned int connectionToUse = 0;
  networkAddress targetAddress = socketToUse->connections[connectionToUse].remoteAddress;
  transPORT targetPort = socketToUse->connections[connectionToUse].remotePort;
  unsigned int seqMsg = socketToUse->connections[connectionToUse].outboundSeqMsg;

  //char* msgToSplit = "SPLIT ME PLEASE!!!!"; //"12345678abcdefghYNYNYNYN"; //"ASDFasdf"; //"ASDF\n";
  //unsigned int msglen = 20;
  int cpa = MAX_PAYLOAD;

  int numFragments = msglen / MAX_PAYLOAD;
  if(numFragments*MAX_PAYLOAD < msglen) {numFragments++;};

  if(msglen < cpa) {cpa = msglen;}
  //logLine(succes, "Splitting message %s with msglen %d, cpa %d, numFragments: %d\n", msgToSplit, msglen, cpa, numFragments);
  //logLine(succes, "Splitting message %s with msglen %d, cpa %d, numFragments: %d\n", "HEH", 4, 4, 1);
  //logLine(succes, "test\n");

  //First fragment. Set to be the fourth message (3).
  O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
  O->otherHostAddress = targetAddress; //111;
  O->segment.is_first = 1;
  O->segment.is_control = 0;
  O->segment.seqMsg = seqMsg; //4;
  O->segment.seqPayload = numFragments-1;
  O->segment.senderport = socketToUse->port; //2;
  O->segment.receiverport = targetPort; //0; //This one should correspond to the open connection
  //O->segment.msg.data = "TEST.\n";
  O->segment.aux = msglen;
  memcpy(&(O->segment.msg.data), msgToSplit, cpa);
  memcpy(&(O->seg.data), msgToSplit, cpa);
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
  logLine(trace, "   %p, %p\n", &(O->segment.msg)+(i), msgToSplit+(i*MAX_PAYLOAD));
  while(true) {
  //for(int i = MAX_PAYLOAD; i < msglen; i += MAX_PAYLOAD) {
    //Submit the previous one.
    //Signal(TL_ReceivingQueueOffer, O); //Is used in the testing version.
    EnqueueFQ( NewFQE( (void *) O ), sendingBuffQueue);

    i++; //Important that this comes first.

    if(i >= numFragments) {
      break;
    }

    logLine(debug, "Building additional fragment with seqPayload: %d\n", i);

    O = (TL_OfferElement*) malloc(sizeof(TL_OfferElement));
    O->otherHostAddress = targetAddress; //111;
    O->segment.is_first = 0;
    O->segment.is_control = 0;
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

  logLine(trace, "   %p, %p\n", &(O->segment.msg)+(i*MAX_PAYLOAD), msgToSplit+(i*MAX_PAYLOAD));
    memcpy(&(O->segment.msg.data), msgToSplit+(i*MAX_PAYLOAD), MAX_PAYLOAD);
  memcpy(&(O->seg.data), msgToSplit+(i*MAX_PAYLOAD), MAX_PAYLOAD);
  logLine(debug, "src: %s, dest: %s\n", msgToSplit+(i*MAX_PAYLOAD), O->segment.msg.data);
    /*O->segment.msg.data[0] = 'P';
    O->segment.msg.data[1] = 'A';
    O->segment.msg.data[2] = 'R';
    O->segment.msg.data[3] = 'T';
    O->segment.msg.data[4] = '1';
    O->segment.msg.data[5] = ' ';
    O->segment.msg.data[6] = '-';
    O->segment.msg.data[7] = ' ';*/
    //Signal(TL_ReceivingQueueOffer, O);

    logLine(debug, "fragment built with: %d, %d, %d\n", O->segment.seqMsg, O->segment.seqPayload, cpa);

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

      logLine(trace, "TL: Offering sending queue to NL.\n");
      NL_OfferSendingQueue(&sendingQueue);

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
  logLine(trace, "   %p, %p\n", &(offer->segment.msg)+(i), msgToSplit+(i*MAX_PAYLOAD));
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

  logLine(trace, "   %p, %p\n", &(offer->segment.msg)+(i*MAX_PAYLOAD), msgToSplit+(i*MAX_PAYLOAD));
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
  free(reqp); //good boi
  return ret;
}
