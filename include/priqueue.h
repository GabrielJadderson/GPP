/* 	$Id: priqueue.h,v 1.4 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: En generisk prioritetskø. Primær er den til brug ved fejldatakøen
 * i subnetpakken, men den kan også anvendes af brugeren.
 *
 * Steffen Schmidt.
 */
#ifndef _PRIQUEUE_H_
#define _PRIQUEUE_H_

/*
 * Type definitioner:
 */
typedef struct PQES *PriQueueEntry;
typedef struct PQS  *PriQueue;
typedef struct PQS
{
  PriQueueEntry first;
} PriQueueStruct;

typedef struct PQES
{
  int      	key;
  void         *val;
  PriQueueEntry prev;
  PriQueueEntry next;
} PriQueueEntryStruct;

/*
 * Prototyper:
 */
/* Allokerer og initaliserer en prioritets kø */
PriQueue InitializePQ(void);

/* Tester om koeen er tom (1 = tom, 0 = ikke tom) */
int EmptyPQ(PriQueue Q);

/* Inds�tter element i prioritets køen */
void InsertPQ(PriQueueEntry e, PriQueue Q);

/* Finder mindste element i køen, returnerer dette*/
PriQueueEntry FindMinPQ(PriQueue Q);

/* Finder mindste element i køen, returnerer værdien af nøglen*/
int FindMinKeyPQ(PriQueue Q);

/* Finder mindste element i koeen, returnerer dette, og fjerner det fra køen*/
PriQueueEntry DeleteMinPQ(PriQueue Q);

/* De-allokerer prioritetskøen */
void DeletePQ(PriQueue Q);

/* Allokerer et nyt element i køen */
PriQueueEntry NewPQE(int key, void *val);

/* De-allokerer et element i køen */
void DeletePQE(PriQueueEntry e);

/* Returnerer værdien af nøglen */
int KeyOfPQE(PriQueueEntry e);

/* Returnerer værdien af et element */
void *ValueOfPQE(PriQueueEntry e);

/* Fjerner et element fra prioritetskøen. Ikke en 'ren' ADT operation */
int RemovePQE(PriQueueEntry e, PriQueue Q);

/* Udskriver indholdet af et element, vha. udskrift funktionen 'func' */ 
void PrintPQE(PriQueueEntry e, void (*func)(void *));

/* Udskriver indholdet af en kø, vha. udskrift funktionen 'func' */
void PrintPQ(PriQueue Q, void (*func)(void *));
#endif
