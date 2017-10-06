/* 	$Id: fifoqueue.c,v 1.6 1999/09/04 11:33:36 dim Exp $ 
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: Implementation af en fifokø.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: fifoqueue.c,v 1.6 1999/09/04 11:33:36 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>

#include "fifoqueue.h"

/*
 * Initialiserer en fifokø.
 */
FifoQueue InitializeFQ(void)
{
  FifoQueue Q = (FifoQueue) malloc (sizeof(FifoQueueStruct));
  Q->first = NULL;
  Q->last  = NULL;
  return Q;
}

/*
 * Tester om en k� er tom, tom = 1, ikke tom = 0
 */
inline int EmptyFQ(FifoQueue Q)
{
  if (Q == NULL)
    return (Q == NULL);
  else
    return (Q->first == NULL);
}

/*
 * Inds�tter et element i fifo-k�en
 */
void EnqueueFQ(FifoQueueEntry e, FifoQueue Q)
{
  /* Fifokøen er slet ikke initialiseret */
  if (Q == NULL)
    return;

  /* placeringen til det nye element findes */
  if (EmptyFQ(Q))    /* listen er tom */
    {
      Q->first = e;
      Q->last  = e;
    }
  else               /* tilføjes til enden af listen */
    {
      Q->last->next  = e;
      e->prev        = Q->last;
      Q->last        = e;
    }
  return;
}

/*
 * Fjerner det første element fra fifokøen.
 */
FifoQueueEntry DequeueFQ(FifoQueue Q)
{
  FifoQueueEntry e;

  if (EmptyFQ(Q))
    return NULL;
  else
    {
      /* Det første element fjernes fra listen */
      e = Q->first;

      /* er der et eller flere elementer tilbage? */
      if ( e->next == NULL)
	{
	  /* der er kun et element tilbage */
	  Q->first = NULL;
	  Q->last  = NULL;
	}
      else
	{
	  /* elementet bagved det aktuelle element opdateres */
	  Q->first       = e->next;
	  Q->first->prev = NULL; 
	}
      e->next  = NULL;
      e->prev  = NULL;
      return e;
    }
}

/*
 * Returnerer en kopi af første element i køen (det næste til at blive
 * 'de-queued'
 */
FifoQueueEntry FirstEntryFQ(FifoQueue Q)
{
  if (EmptyFQ(Q))
    return NULL;
  else
    /* laver en kopi af første element som så returneres */
    return NewFQE(Q->first->val);
}

/*
 * Deallokerer en fifokø.
 */
void DeleteFQ(FifoQueue Q)
{
  while(DequeueFQ(Q) != NULL);
  
  free(Q);

  Q = NULL;
  return;
}

/*
 * Allokerer plads til et nyt element;
 */
FifoQueueEntry NewFQE(void *val)
{
  FifoQueueEntry e = (FifoQueueEntry) malloc (sizeof(FifoQueueEntryStruct));

  e->prev    = NULL;
  e->next    = NULL;
  e->val     = val;
  return e;
}

/*
 * Deallokerer et element fra fifokøen
 */
inline void DeleteFQE(FifoQueueEntry e)
{
  free(e);

  e = NULL;
}

/*
 * Returnerer værdien af et element i fifokøen.
 */
inline void *ValueOfFQE(FifoQueueEntry e)
{
  if (e == NULL)
    return NULL;
  else
    return e->val;
}

/*
 * Udskriver et element, ved brug af udskriftsfunktionen fra parameterlisten
 */ 
void PrintFQE(FifoQueueEntry e, void (*func)(void *))
{
  if (e == NULL)
    func( NULL );
  else
    func( e->val );
  
  return;
}

/*
 * Udskriver en fifokø, ved brug af udskriftsfunktionen fra parameterlisten
 */ 
void PrintFQ(FifoQueue Q, void (*func)(void *))
{
  FifoQueueEntry CurrentE;

  if (!EmptyFQ(Q))
    {
      CurrentE = Q->first->next;
      PrintFQE(Q->first, func);
      while (CurrentE != NULL)
	{
	  PrintFQE(CurrentE, func);
	  CurrentE = CurrentE->next;
	}
    }
  return;
}
