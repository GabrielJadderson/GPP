

//Block until the application has a connection on this port to another host.
//Returns the id of the connection on succes, -1 on error.
//This function doesn't guarantee that only one connection has been made at the time this function returns.
//int listen(TLSocket *socket) { //[PJ] This line contains some invisible character? Gives me a compile error.
int listen(TLSocket *socket) {
  if (socket->valid == 1) {
      socket->listening |= 1;
      //while(socket->listening == 1) {
      while(socket->listenConnection == CONNECTION_PENDING) {
        logLine(succes,"Application is listening. listening: %d, connection: %d\n", socket->listening, socket->listenConnection);
        sleep(1);
      }
      socket->listening = 0;
      return socket->listenConnection;
  } else {
      return -1;
  }
}

/*Connects to a remote host on address addr on port port.
 * Gives back the id of the connection. On error, gives a negative error code.
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
  
  for(int i = 0; i < 3 && (connReq->connectionid == CONNECTION_PENDING || socket->connections[connReq->connectionid].pending) == 1; i++) {
  //while(connReq->connectionid == CONNECTION_PENDING || socket->connections[connReq->connectionid].pending == 1) {
    logLine(succes,"Application is waiting for connection to be established.\n");
    sleep(1);
  }

  int res = connReq->connectionid;
  logLine(succes, "res: %d\n", res);
  
  if(socket->connections[res].valid == 0) {
    socket->connections[res].pending = 0; //Disable waiting for this connection.
    return -2; //Timeout
  } else if(res == CONNECTION_FAILURE) {
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

  //register the connection disabled. //[PJ] This is also done in TL, but when done here, it is certain that thread scheduling isn't a problem.
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
