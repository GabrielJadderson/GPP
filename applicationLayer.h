
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
int listen(TLSocket *socket);

//Connects to a remote host on address addr on port port.
//Gives back the id of the connection.
unsigned int connect(TLSocket *socket, networkAddress addr, transPORT port);

int disconnect(TLSocket *socket, unsigned int connectionid);

int send(TLSocket *socket, unsigned int connectionid, char *data, unsigned int length);

//On succes, gives a pointer to a byte array.
//On failure, gives NULL. This means that there are current haven't arrived any more messages.
//Can be used in an assign-and-check-NULL loop.
char* receive(TLSocket *socket, unsigned int connectionid);






































#endif /* include guard: AL_H_ */

