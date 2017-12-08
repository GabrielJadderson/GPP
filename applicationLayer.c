

//Block until the application has a connection on this port to another host.
//Returns the id of the connection on succes, -1 on error.
//int listen(TLSocket *socket) { //[PJ] This line contains some invisible character? Gives me a compile error.
int listen(TLSocket *socket) {
  if (socket->valid == 1) {
      socket->listening = 1;
      while(socket->listening == 1) {
        logLine(trace,"Application is listening\n");
      }
      return socket->listenConnection;
  } else {
      return -1;
  }
}

/*Connects to a remote host on address addr on port port.
 * Gives back the id of the connection. In case of an error returns -1;
 */
int connect(TLSocket *socket, networkAddress addr, transPORT port) {
  if(!socket->valid) {
    return -1;
  }
  
  ALConnReq* connReq = malloc(sizeof(ALConnReq));

  connReq->port = port;
  connReq->sock = socket;
  connReq->netAddress = addr;
  connReq->connectionid = CONNECTION_PENDING;
  
  Signal(AL_Connect, connReq);
  
  while(connReq->connectionid == CONNECTION_PENDING || socket->connections[connReq->connectionid].pending == 1) {
    logLine(trace,"Application is waiting for connection to be established.\n");
  }

  int res = connReq->connectionid;
  
  if(res == CONNECTION_FAILURE || socket->connections[res].valid == 0) {
    return -1; //Failure.
  }
  
  free(connReq);
  return res;
}


 //Tries to disconnect the connection specified by the socket and the connection id.
 // returns 0 if sucessfully disconnected otherwise, -1
int disconnect(TLSocket *socket, unsigned int connectionid) {

  if (!socket || connectionid < 0) {
      logLine(error, "ApplicationLayer: Failed to disconnect. socket null");
      return -1;
  }
  if (socket->connections[connectionid].valid != 1) {
      return -1;
    }

  //register the connection disabled.
  socket->connections[connectionid].valid = 0;

  //just make a struct and pass it to the TL
  ALDisconnectReq* disconnectReq = malloc(sizeof(ALDisconnectReq));
  disconnectReq->sock = socket;
  disconnectReq->connectionid = connectionid;

  Signal(AL_Disconnect, disconnectReq);
  return 0;
}

int send(TLSocket *socket, unsigned int connectionid, char *data, unsigned int length) {
  ALMessageSend *MS = (ALMessageSend*) malloc(sizeof(ALMessageSend));
  MS->socketToUse = socket;
  MS->connectionid = connectionid;
  MS->message = data;
  MS->length = length;

  Signal(AL_Send, MS);

  return 0;
}

//On succes, gives a pointer to a byte array.
//On failure, gives NULL. This means that there are current haven't arrived any more messages.
//Can be used in an assign-and-check-NULL loop.
char* receive(TLSocket *socket, unsigned int connectionid) {
  ALMessageSend *MS = (ALMessageSend*) malloc(sizeof(ALMessageSend));

  MS->socketToUse = socket;
  MS->connectionid = connectionid;
  MS->message = NULL;
  MS->length = 0;
  MS->aux = 1;

  Signal(AL_Receive, MS);

  while(MS->aux == 1) {
    logLine(trace, "waitin'"); //[PJ] logLines are appearently the most fitting calls to make here. Probably because they interact with an outside resource.
  }

  logLine(debug, "AL RECEIVE: %p, %d, %p, %d, %d\n", MS->socketToUse, MS->connectionid, MS->message, MS->length, MS->aux);

  free(MS);
  return MS->message;
}
