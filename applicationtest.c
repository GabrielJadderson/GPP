
//TODO: Add includes and the author header.

#include "transportLayer.h"
#include "applicationLayer.h"

void textChatStatic1(void) {
  //[PJ] As good a place as any to check this.
  //logLine(succes, "framekind size: %d\n", sizeof(frame_kind));
  
  
  logLine(succes, "WEE.\n");
  //Signal(AL_Receive, NULL);
  
  TLSocket* socket = TL_RequestSocket(1);
  
  logLine(succes, "Received socket properties:\n");
  logLine(succes, "Validity: %d\n", socket->valid);
  
  if(socket->valid) {
    logLine(succes, "port: %d\n", socket->port);
  }
  
  
  //A HACK! SHOULD USE A LEGIT CONNECTION!
  /*
  socket->connections[0].valid = 1;
  socket->connections[0].remoteAddress = 212;
  socket->connections[0].remotePort = 0;
  socket->connections[0].outboundSeqMsg = 0;
  socket->connections[0].msgListHead = NULL;
  socket->connections[0].msgListTail = NULL;
  */
  
  //sleep(1); //Give the "server" a chance to start listening before we try to connect to it.
  
  int conid = -2;
  
  while(conid == -2) {
    if(ThisStation == 1) {
      conid = connect(socket, 212, 0);
    } else {
      conid = connect(socket, 111, 0);
    }
  }
  
  
  logLine(succes, "<*_*<: established connection at id: %d on port %d\n", conid, socket->port);
  
  //sleep(1);
  
  //Try sending a message to see the debugging messages.
  /*ALMessageSend *MS = (ALMessageSend*) malloc(sizeof(ALMessageSend));
  MS->socketToUse = socket;
  MS->connectionid = 0;
  MS->message = "SPLIT ME PLEASE!!!!";
  MS->length = 20;
  
  Signal(AL_Send, MS);*/
  //Signal(AL_Receive, NULL);
  /*
  send(socket, 0, "SPLIT ME PLEASE #01", 20);
  send(socket, 0, "SPLIT ME PLEASE #02", 20);
  send(socket, 0, "SPLIT ME PLEASE #03", 20);
  send(socket, 0, "SPLIT ME PLEASE #04", 20);
  send(socket, 0, "SPLIT ME PLEASE #05", 20);
  send(socket, 0, "SPLIT ME PLEASE #06", 20);
  send(socket, 0, "SPLIT ME PLEASE #07", 20);
  send(socket, 0, "SPLIT ME PLEASE #08", 20);
  send(socket, 0, "SPLIT ME PLEASE #09", 20);
  send(socket, 0, "SPLIT ME PLEASE #10", 20);
  */
  
  send(socket, conid, "SPLIT ME PLEASE #01", 20);
  send(socket, conid, "SPLIT ME PLEASE #02", 20);
  send(socket, conid, "SPLIT ME PLEASE #03", 20);
  send(socket, conid, "SPLIT ME PLEASE #04", 20);
  send(socket, conid, "SPLIT ME PLEASE #05", 20);
  send(socket, conid, "SPLIT ME PLEASE #06", 20);
  send(socket, conid, "SPLIT ME PLEASE #07", 20);
  send(socket, conid, "SPLIT ME PLEASE #08", 20);
  send(socket, conid, "SPLIT ME PLEASE #09", 20);
  send(socket, conid, "SPLIT ME PLEASE #10", 20);
  
  
  sleep(1);
  logLine(succes, "This is before that disconnect!\n");
  if(ThisStation == 1) {
    disconnect(socket, 0);
  }
  logLine(succes, "This is after that disconnect!\n");
  
  sleep(50); //Give some time to finish.
  //Stop();
  
}


void textChatStatic2(void) {
  
  
  logLine(succes, "WEE.\n");
  //Signal(AL_Receive, NULL);
  
  sleep(5);
  TLSocket* socket = TL_RequestSocket(0);
  
  logLine(succes, "Received socket properties:\n");
  logLine(succes, "Validity: %d\n", socket->valid);
  
  if(socket->valid) {
    logLine(succes, "port: %d\n", socket->port);
  }
  
  
  //A HACK! SHOULD USE A LEGIT CONNECTION!
  /*socket->connections[0].valid = 1;
  socket->connections[0].remoteAddress = 111;
  socket->connections[0].remotePort = 0;
  socket->connections[0].outboundSeqMsg = 0;
  socket->connections[0].msgListHead = NULL;
  socket->connections[0].msgListTail = NULL;*/
  
  int conid = listen(socket);
  
  /*if(ThisStation == 1) {
    disconnect(socket, conid); //Just to see if it receives messages anyway.
    //sleep(50); //Just stall this one.
  }*/
  
  logLine(succes, ">*_*>: established connection at id: %d on port %d\n", conid, socket->port);
  
  sleep(1);
  
  //Try sending a message to see the debugging messages.
  /*ALMessageSend *MS = (ALMessageSend*) malloc(sizeof(ALMessageSend));
  MS->socketToUse = socket;
  MS->connectionid = 0;
  MS->message = "SPLIT YOU PLEASE!!!";
  MS->length = 20;
  
  Signal(AL_Send, MS);*/
  //Signal(AL_Receive, NULL);
  
  //send(socket, 0, "SPLIT YOU PLEASE!!!", 20);
  //send(socket, conid, "SPLIT YOU PLEASE!!!", 20);
  
  /*
  char* othermessage = receive(socket, 0);
  
  while(othermessage == NULL) {
    sleep(1);
    othermessage = receive(socket, 0);
  }
  
  logLine(succes, "Received from other host: %s\n", othermessage);
  */
  
  
  char* othermessage = NULL;
  int numreceived = 0;
  
  while(numreceived < 10) {
    
    othermessage = receive(socket, 0);
    while(othermessage == NULL) {
      sleep(1);
      othermessage = receive(socket, 0);
    }
    
    logLine(succes, "Received from other host %s\n", othermessage);
    numreceived++;
    othermessage = NULL; //Just to be sure
    
    /*if(ThisStation == 1 && numreceived == 1) {
      disconnect(socket, conid);
    }*/
    
  }
  
  if(ThisStation == 1) {sleep(60);}
  
  logLine(succes, "APPLICATION: RECEIVED ALL MESSAGES FROM OTHER HOST.\n");
  
  sleep(50); //Give some time to finish.
  //Stop();
  
}



void socketTest(void) {
  
  TLSocket* socket = TL_RequestSocket(0);
  
  logLine(succes, "Received socket properties:\n");
  logLine(succes, "Validity: %d\n", socket->valid);
  
  if(socket->valid) {
    logLine(succes, "port: %d\n", socket->port);
  }
  
  
  TLSocket* socket2 = TL_RequestSocket(1);
  
  logLine(succes, "Received socket properties:\n");
  logLine(succes, "Validity: %d\n", socket2->valid);
  
  if(socket2->valid) {
    logLine(succes, "port: %d\n", socket2->port);
  }
  
  
  
}





















