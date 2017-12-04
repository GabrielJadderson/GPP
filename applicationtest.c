
//TODO: Add includes and the author header.

#include "transportLayer.h"
#include "applicationLayer.h"

void textChatStatic1(void) {
  
  
  logLine(succes, "WEE.\n");
  //Signal(AL_Receive, NULL);
  
  TLSocket* socket = TL_RequestSocket(3);
  
  logLine(succes, "Received socket properties:\n");
  logLine(succes, "Validity: %d\n", socket->valid);
  
  if(socket->valid) {
    logLine(succes, "port: %d\n", socket->port);
  }
  
  
  //A HACK! SHOULD USE A LEGIT CONNECTION!
  socket->connections[0].valid = 1;
  socket->connections[0].remoteAddress = 111;
  socket->connections[0].remotePort = 0;
  socket->connections[0].outboundSeqMsg = 0;
  socket->connections[0].msgListHead = NULL;
  socket->connections[0].msgListTail = NULL;
    
  
  //Try sending a message to see the debugging messages.
  ALMessageSend *MS = (ALMessageSend*) malloc(sizeof(ALMessageSend));
  MS->socketToUse = socket;
  MS->connectionid = 0;
  MS->message = "SPLIT ME PLEASE!!!!";
  MS->length = 20;
  
  Signal(AL_Send, MS);
  //Signal(AL_Receive, NULL);
  
  
  //sleep(5); //Give some time to finish.
  //Stop();
  
}
























