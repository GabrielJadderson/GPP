/*
* concurrentFIFOQueue.h
*
*  Created on: Nov 01, 2017
*  Authors: Patrick Jakobsen (pajak16), Gabriel Jadderson (gajad16)
*
*/

#include "fifoqueue.h"
#include "subnetsupport.h" //mlock_t is defined in here

#ifndef concurrentFIFOQueue_H_
#define concurrentFIFOQueue_H_

typedef struct {
  mlock_t *lock;
  FifoQueue queue;
} ConcurrentFifoQueue;

/*
*   Functions
*/

ConcurrentFifoQueue CFQ_Init() {
  ConcurrentFifoQueue q;
  q.queue = InitializeFQ();
  q.lock = malloc(sizeof(mlock_t));
  Init_lock(q.lock);
  return q;
}


#endif /* include guard: concurrentFIFOQueue_H_ */
