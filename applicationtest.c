
//TODO: Add includes and the author header.

#include "transportLayer.h"
#include "applicationLayer.h"

void textChatStatic1(void) {
  
  
  logLine(succes, "WEE.\n");
  TLSocket* socket = TL_RequestSocket(3);
  
  logLine(succes, "Received socket properties:\n");
  logLine(succes, "Validity: %d\n", socket->valid);
  
  if(socket->valid) {
    logLine(succes, "port: %d\n", socket->port);
  }
  
  
  
  
  
  Stop();
  
}
























