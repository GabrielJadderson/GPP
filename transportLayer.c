
#include "events.h"
#include "concurrentFIFOQueue.h"

#include "transportLayer.h"
#include "networkLayer.h"

void fake_transportLayer() {
  
  //char* buffer = "HEH.\n";
  ConcurrentFifoQueue q = CFQ_Init();
  
  TL_OfferElement elem;
  elem.receiver = 42;
  //elem.seg = "HEH.\n";
  elem.seg.data[0] = 'H';
  elem.seg.data[1] = 'E';
  elem.seg.data[2] = 'H';
  elem.seg.data[3] = '.';
  elem.seg.data[4] = '\n';
  elem.seg.data[5] = '\0';
  
  Lock(q.lock);
  
  EnqueueFQ( NewFQE( (void *) &elem ), q.queue );
  
  Unlock(q.lock);
  logLine(trace, "TL: Offering queue.\n");
  NL_OfferSendingQueue(&q);
  
  while(Trylock(q.lock) != 0) {
    logLine(trace, "TL: Waiting for unlocking of queue.\n");
    sleep(1);
  }
  
  logLine(succes, "TL: NL Released the sending queue.\n");
  
  Stop();
  
}



