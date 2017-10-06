/* 	$Id: priqueue.c,v 1.5 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: En generisk prioritetskø. Primær er den til brug ved fejldatakøen
 * i subnetpakken, men den kan også anvendes af brugeren.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: priqueue.c,v 1.5 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>

#include "priqueue.h"

/*
 * Initaliserer en prioritetskø.
 */
PriQueue InitializePQ(void)
{
  PriQueue Q = (PriQueue) malloc (sizeof(PriQueueStruct));

  Q->first = NULL;
  return Q;
}

/*
 * Tester om køen er tom (1 = tom, 0 = ikke tom)
 */
inline int EmptyPQ(PriQueue Q)
{
  if (Q == NULL)
    return (Q == NULL);
  else
    return (Q->first == NULL);
}

/*
 * Indsætter et element i prioritetskøen.
 */
void InsertPQ(PriQueueEntry e, PriQueue Q)
{
  PriQueueEntry CurrentE;
  int FOUND = 0; /* er der fundet en plads eller ej */

  /* Prioritetskøen/elementet er  ikke initialiseret */
  if ((Q == NULL) || (e == NULL ))
    return;

  CurrentE = Q->first;

  /* placeringen til det nye element findes */
  if (EmptyPQ(Q))
    Q->first = e;
  else
    while(!FOUND)
      {
	/* placeres før nuværende element */
	  if (e->key < CurrentE->key)
	    {
	      if (CurrentE->prev == NULL)
		Q->first = e;             /* f�rste element i listen */
	      else
		CurrentE->prev->next = e;

	      /* midt i listen  */
	      e->next = CurrentE;
	      e->prev = CurrentE->prev;
	      CurrentE->prev = e;
	      FOUND = 1;
	    }

	  /* som sidste element */
	  if ((CurrentE->next == NULL) && !FOUND)
	    {
	      CurrentE->next = e;
	      e->prev = CurrentE;
	      FOUND = 1;
	    }
	  else
	    CurrentE = CurrentE->next;
      }
  return;
}

/*
 * Returnerer værdien af elementes nøgle, -1 hvis elementet ikke er defineret
 */
inline int KeyOfPQE(PriQueueEntry e)
{
  if (e == NULL)
    return -1;
  else
    return e->key;
}

/*
 * Returnerer værdien af et element i køen
 */
inline void *ValueOfPQE(PriQueueEntry e)
{
  if (e == NULL)
    return NULL;
  else
    return e->val;
}

/*
 * Returnerer en kopi af elementet med den mindste nøgle
 */
PriQueueEntry FindMinPQ(PriQueue Q)
{
  if (EmptyPQ(Q))
    return NULL;
  else
    /* laver en kopi af første element som så returneres */
    return NewPQE(Q->first->key, Q->first->val);
}

/*
 * Returnerer værdien af den mindste nøgle, og -1 hvis den ikke eksisterer
 */
inline int FindMinKeyPQ(PriQueue Q)
{
  if (EmptyPQ(Q))
    return -1;
  else
    return Q->first->key;
}

/*
 * Fjerner det mindste element fra køen
 */
PriQueueEntry DeleteMinPQ(PriQueue Q)
{
  PriQueueEntry e;

  if (EmptyPQ(Q))
    return NULL;
  else
    {
      /* Det første element fjernes fra listen */
      e        = Q->first;

      /* Starten af listen opdateres */
      Q->first = Q->first->next;

      if (!(Q->first == NULL)) /* Er listen tom */  
	Q->first->prev = NULL;

      /* Elementet returneres */
      e->next  = NULL;
      e->prev  = NULL;
      return e;
    }
}

/*
 * Deallokerer en prioritetskø
 */
void DeletePQ(PriQueue Q)
{
  while(DeleteMinPQ( Q ) != NULL);

  Q = NULL;
  return;
}

/*
 * Allokerer et nyt element til prioritetskøen
 */
PriQueueEntry NewPQE(int key, void *val)
{
  PriQueueEntry e = (PriQueueEntry) malloc (sizeof(PriQueueEntryStruct));

  e->key   = key;
  e->val   = val;
  e->prev  = NULL;
  e->next  = NULL;
  return e;
}

/*
 * Deallokerer et element
 */
inline void DeletePQE(PriQueueEntry e)
{
  e->prev = NULL;
  e->next = NULL;
  free(e);
  e = NULL;
}

/*
 * Fjerner et specifikt kø-element. Dette er ikke en 'ren' kø-operation, dvs.
 * den strider mod god opførsel i en ADT som prioritetskøen. Ikke desto mindre
 * er den brugbar når man har en pointer til det ønskede element.
 *
 *                -------->> SKAL BRUGES MED OMTANKE! <<--------
 *
 * Returnerer -1 ved fejl og 0 ved succes.
 */
int RemovePQE(PriQueueEntry e, PriQueue Q)
{
  if ( EmptyPQ( Q ) || ( e == NULL))
    return -1;

  /* Opdaterer efterfølgers 'prev' pointer */
  if (e->next != NULL)
    {
      if (e->next->prev == e)  /* er e e's efterfølgers forgænger? (!?) */
	e->next->prev = e->prev;
      else
	return -1;
    }

  /* Opdaterer forgængers (evt. Q->first) 'next' pointer */
  if (e->prev == NULL)                 /* første element */
    {
      if (Q->first == e)           /* Står e først i køen */
	Q->first = e->next;
      else
	return -1;
    }
  else
    {
      if (e->prev->next == e) /* er e e's forgængers efterfølger? */
	e->prev->next = e->next;
      else
	return -1;
    }

  /* Deakllokerer elementet */
  DeletePQE( e );

  return 0;
}

/*
 * Udskriver et element, ved brug af udskriftsfunktionen fra parameterlisten
 */ 
void PrintPQE(PriQueueEntry e, void (*func)(void *))
{
  if (!(e == NULL))
    func( e );
  
  return;
}

/*
 * Udskriver en prioritetskø, ved brug af udskriftsfunktionen fra parameterlisten
 */ 
void PrintPQ(PriQueue Q, void (*func)(void *))
{
  PriQueueEntry CurrentE;
  
  if (!EmptyPQ(Q))
    {
      CurrentE = Q->first;
      while (CurrentE != NULL)
        {
          PrintPQE(CurrentE, func);
          CurrentE = CurrentE->next;
        }
    }
  return;
}
