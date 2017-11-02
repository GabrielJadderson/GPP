/* 	$Id: subnetsupport.h,v 1.10 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Indeholder prototyper til support funktionerne.
 *
 * Steffen Schmidt.
 */
#ifndef _SUBNETSUPPORT_H_
#define _SUBNETSUPPORT_H_

#include <pthread.h>
#include "fifoqueue.h"

#define timeout  0x00000002        /* systemets timeoutsignal */

/*
 * Type definitioner
 */
typedef struct LBS       *LogBuf;
typedef struct 
{
  pthread_mutex_t  mutex;
} mlock_t;

typedef struct
{
  FifoQueue          BufQ;
  pthread_mutex_t    lock;
} BufQControlStruct;

typedef struct LBS
{
  char            *name;
  int              pos;
  int              num;
  char            *logbuf;
  pthread_mutex_t  lock;
} LogBufStruct;

/*
 * Funktioner til ops�tning af kritiske regioner.
 */
int Init_lock(mlock_t *lock);                                     /* Initerer lås. */
int Destroy_lock(mlock_t *lock);                                 /* Nedlægger lås. */
int Lock(mlock_t *lock);                                                 /* Låser. */
int Trylock(mlock_t *lock);                                   /* Forsøger at låse. */
int Unlock(mlock_t *lock);                                            /* Låser op. */

/*
 * Funktioner til tidtagning. Returnerer antal millisekunder siden stationen 
 * startede. S�tter timer til 'delay' antal millisekunder med beskeden '*msg'.
 * Stopper timer.
 */
long GetTime(void);                                                /* Henter tid. */ 
unsigned int SetTimer(int delay, void *msg);    /* Sætter timer, og returnerer id */
int StopTimer(unsigned int id, void **msg);     /* stopper timer med det givne id */
unsigned int GetLastTid();        /* Giver timer-id på sidst hentet timeout event */

/*
 * Funktioner til at logge data med. 
 */
LogBuf InitializeLB(const char *name);  /* Indsættes i kø, så FlushAll er muligt. */
LogBuf NewLogBuffer(const char *name);      /* Ny logbuffer. Bliver IKKE flushed. */
void Log(LogBuf b, const char *data, int legnth);  /* Logger 'data' i buffer 'b'. */
void SyncLog(const char *loginfo, int length);           /* Logger synkroniseret. */

void PrintLog(LogBuf b);                       /* Udskriver en specfik logbuffer. */
void FlushAllLog(void);    /* Udskriver alle bufferer indsat via 'InitializedLB'. */

/*
 * Returnerer en pointer til en streng med navnet som processen blev startet med.
 */
char *GetProcessName(void);
#endif
