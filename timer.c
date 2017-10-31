/* 	$Id: timer.c,v 1.6 1999/09/04 12:03:30 dim Exp $	
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: processen tager sig af at sende evt. timeout signaler.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: timer.c,v 1.6 1999/09/04 12:03:30 dim Exp $";
*/
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct  GC;           /* samlet kontrolstruktur */
extern StateControlStruct   SC;           /* Systemets tilstand */
extern TimeoutControlStruct TC;           /* kontrol af timeouts */
extern pthread_key_t        id;           /* til identifikation af threaden */
extern log_type             LogStyle;     /* hvordan skal systemet føre log? */

void *timer(void *arg)
{
  register int temp;
  int FLAG;
  PriQueueEntry  pqe;
  TimeoutElement te;
  struct timeval tv;
  struct timezone tz;
  const char errormsg[] = "timer - reading systemtime.\n";

  /*
   * threaden 'husker' sit eget id og navn.
   */
  pthread_setspecific( id, malloc( sizeof( void * )) );
  *((intptr_t *)pthread_getspecific( id )) = (intptr_t)arg;

  GC.ThArr[(intptr_t)arg]->name = strcpy( (char *) malloc (sizeof(char)* 22), 
 				     "(SYSTEM) timer"); 

  /* initialiserer timeout køen */
  TC.timeoutQ = InitializePQ();

  /* venter på et GO fra hovedprogrammet */
  LOCK( &SC.systemlock, timer );
  pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, timer );

  while(1)
    {
      /* undersøger om stationen er klar eller ved at lukke ned (ved fejl) */
      SYSTEMOK( timer );

      LOCK( &TC.lock, timer );

      /* Hvis der ikke er nogen ventende timeout, så vent til der kommer nogen */
      if (EmptyPQ( TC.timeoutQ ))
	pthread_cond_wait( &TC.signal, &TC.lock );

      /* henter tid. Standser ved fejl. */
      if ( gettimeofday( &tv, &tz ) )
	{
	  LOGINFO( errormsg, strlen(errormsg) );
	  perror( errormsg );
	  Stop();
	}

      /* sætter aktuel systemtid */
      temp = ((tv.tv_sec*1000000 + tv.tv_usec) / 1000 - GC.systime);

      /* er tiden inde til at sende signaler? */
      if (temp > FindMinKeyPQ( TC.timeoutQ ))
	{
	  FLAG = EmptyPQ( TC.timeoutQ );        /* 1 = tom */

	  /* elementer fjernes fra timeout-køen */
	  while ((temp > FindMinKeyPQ( TC.timeoutQ )) && !FLAG)
	    {
	      pqe = DeleteMinPQ( TC.timeoutQ );
	      
	      te = (TimeoutElement)ValueOfPQE( pqe );

	      /* fjern timeout fra aktiv listen */
	      removeTimeout( lookupTimeoutId( te->id ));
	      
	      /* sender timeout-signaler */
	      if (te->cleared == FALSE)
		Signal( timeout, te );
	      else
		free( te ); /* ellers skal der deallokeres */

	      DeletePQE( pqe );
	      FLAG = EmptyPQ( TC.timeoutQ );
	    }
	}
      UNLOCK( &TC.lock, timer );
      sched_yield(); /* processkift */
    }
#if !defined (__sun)
	  return NULL;
#endif
}
