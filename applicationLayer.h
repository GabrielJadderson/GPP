
#include "transportLayer.h"
#include "networkLayer.h"

#ifndef AL_H_
#define AL_HL


/*
*   Types
*/






/*
*   Event Function Predeclarations
*/

//Block until the application has a connection on this port to another host.
//Returns 0 on succes, other on error.
int listen(transPORT listenPort);

//Connects to a remote host on address addr on port port.
int connect(networkAddress addr, transPORT port);

int disconnect(transPORT ownport, unsigned int connectionid);

int send(connectionid);

int receive();






































#endif /* include guard: AL_H_ */

