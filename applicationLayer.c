

//Block until the application has a connection on this port to another host.
//Returns 0 on succes, other on error.
int listen(TLSocket *socket) {
  socket->listening = 1;
  while(socket->listening == 1) {
      logLine(trace,"Application is listening\n");
  }
  return 0;
}

//Connects to a remote host on address addr on port port.
//Gives back the id of the connection.
unsigned int connect(TLSocket *socket, networkAddress addr, transPORT port) {
  ALConnReq* connReq = malloc(sizeof(ALConnReq));

  connReq->port = port;
  connReq->sock = socket;
  connReq->netAddress = addr;

  Signal(AL_Connect, connReq);
}

int disconnect(TLSocket *socket, unsigned int connectionid) {

}

int send(TLSocket *socket, unsigned int connectionid, unsigned char *data, unsigned int length) {

}

//On succes, gives a pointer to a byte array.
//On failure, gives NULL. This means that there are current haven't arrived any more messages.
//Can be used in an assign-and-check-NULL loop.
unsigned char* receive(TLSocket *socket, unsigned int connectionid) {

}
