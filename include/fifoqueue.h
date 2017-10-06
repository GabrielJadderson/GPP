/* 	$Id: fifoqueue.h,v 1.5 1999/09/04 11:33:23 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Køen laves som en generisk fifokø, således den kan bruges i
 * flere forskellige sammenhæng.
 *
 * Steffen Schmidt.
 */
#ifndef _FIFOQUEUE_H_
#define _FIFOQUEUE_H_

/*
 * Type definitioner:
 */
typedef struct FQES *FifoQueueEntry;
typedef struct FQS  *FifoQueue;

typedef struct FQS
{
  FifoQueueEntry first;
  FifoQueueEntry last;
} FifoQueueStruct;

typedef struct FQES
{
  FifoQueueEntry prev;
  FifoQueueEntry next;
  void           *val;
} FifoQueueEntryStruct;

/*
 * Prototyper:
 */
/* Allokerer og initaliserer en fifokø */
FifoQueue InitializeFQ(void);

/* Tester om k�en er tom (1 = tom, 0 = ikke tom) */
int EmptyFQ(FifoQueue Q);

/* Indsætter element i fifokøen */
void EnqueueFQ(FifoQueueEntry e, FifoQueue Q);

/* Finder mindste element i koeen, returnerer dette, og fjerner det fra køen*/
FifoQueueEntry DequeueFQ(FifoQueue Q);

/* Returnerer det element der bliver fjernet fra køen næste gang*/
FifoQueueEntry FirstEntryFQ(FifoQueue Q);

/* De-allokerer fifok�en */
void DeleteFQ(FifoQueue Q);

/* Allokerer et nyt element i køen */
FifoQueueEntry NewFQE(void *val);

/* De-allokerer et element i køen */
void DeleteFQE(FifoQueueEntry e);

/* returnerer processen */
void *ValueOfFQE(FifoQueueEntry e);

/* Udskriver indholdet af et element, vha. udskrift funktionen 'func' */ 
void PrintFQE(FifoQueueEntry e, void (*func)(void *));

/* Udskriver indholdet af en kø, vha. udskrift funktionen 'func' */
void PrintFQ(FifoQueue Q, void (*func)(void *));

#endif
