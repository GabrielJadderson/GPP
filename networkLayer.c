/*
* networkLayer.c
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

#include "concurrentFIFOQueue.h"
#include "networkLayer.h"
#include "subnetsupport.h"
#include "subnet.h"
#include "fifoqueue.h"
#include "debug.h"


void networkLayer() {
  //Since this network layer isn't 'fake', it has no condition to stop and relies on the above layer to handle this.
  long int events_we_handle = network_layer_allowed_to_send | data_for_network_layer;
  event_t event;
  
  ConcurrentFifoQueue 
  
  while(true) {
    Wait(&event, events_we_handle);
    
    switch(event.type) {
      case network_layer_allowed_to_send:
        
        break;
      
      
    }
  }
}




