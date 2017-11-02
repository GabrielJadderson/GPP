



#ifndef TL_H_
#define TL_H_

typedef struct {
  networkAddress otherHostAddress;
  payload seg;
} TL_OfferElement;


/*
*   Function Predeclarations
*/

void TL_OfferReceivingQueue(ConcurrentFifoQueue *queue);


#endif /* include guard: TL_H_ */
