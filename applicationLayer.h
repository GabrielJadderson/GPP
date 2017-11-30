
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
TLSocket* listen(transPORT listenPort);

//Connects to a 
int connect(networkAddress addr, );

int disconnect();

int send();

int receive();






































#endif /* include guard: AL_H_ */

