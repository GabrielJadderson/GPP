/* 	$Id: Wait.c,v 1.7 1999/09/04 12:11:59 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Ser efter om den ønskede event er sat. Er dette tilfældet
 * returneres straks. Hvis ikke, så venter der på signal på EC.signal.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: Wait.c,v 1.7 1999/09/04 12:11:59 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern EventControlStruct   EC;      /* Kontrolstruktur til styring af events*/
extern StateControlStruct   SC;      /* Systemets tilstand                   */
extern log_type             LogStyle;/* hvordan skal systemet føre log?      */
extern unsigned int         _last_timeout_id;   /* sidst hentede timeout     */
extern pthread_mutex_t      _last_tid_lock;

long int Wait(event_t *result, long int ev_bitmap)
{
  long int res = 0;
  int i, found  = NO;
  FifoQueueEntry fqe;
  TimeoutElement te;

  /* undersøger om stationen er klar eller ved at lukke ned (evt. ved fejl)  */
  SYSTEMOK( Wait );

  /* Som default sætte timer_id til 0. Dette ændres hvis der er et timeout
     signal der fanges. */
  result->timer_id = 0;

  /* Er der nogle af de ønskede events til stede allerede?                   */
  LOCK( &EC.lock, Wait );                  /* der låses om kritisk region    */
  
  if ( ev_bitmap & EC.events )             /* Er der nogle brugbare events?  */
  {
    /* l�ber events igennem; finder mindste først */
    for (i = MAXEVENTNO; i >= 0; i--)
      if ( (ev_bitmap << i) & (EC.events << i) )
	{
	  res = MAXEVENTNO - i;
	  fqe = DequeueFQ( EC.msgQ[res] );

	  /* speciel opførsel hvis der er tale om timeout event */
	  if (EC.timer_active && (timeout == (1 << res)))
	    {
	      /* hent det specielle timeout element og sæt _last_timeout_id */
	      te = (TimeoutElement) ValueOfFQE( fqe );

	      LOCK( &_last_tid_lock, Wait );
	      _last_timeout_id = te->id;
	      UNLOCK( &_last_tid_lock, Wait );

	      result->msg      = te->msg;
	      result->timer_id = te->id;

	      free( te );
	    }
	  else
	    /* ellers skal informationen blot videre gives */
	    result->msg = ValueOfFQE( fqe );

	  DeleteFQE( fqe );

	  if ( EmptyFQ( EC.msgQ[res] ) )
	    EC.events &= ~(1 << res);       /* begivenheden fjernes          */

	  found = YES;
	  break;                             /* standser ved først fundne    */
	}
  }
  UNLOCK( &EC.lock, Wait );                 /* Den kritiske region forlades  */

  /*
   * Hvis der ikke umiddelbart var nogle brugbare events, så sætter processen
   * sig til at vente.
   */
  LOCK( &EC.lock, Wait );
  while (!found)
    {
      /* wait slipper mutex n�r den s�tter sig til at vente            */
      pthread_cond_wait( &EC.signal, &EC.lock );   /* venter på signal */
      /* wait griber mutex efter signalet er fanget.                   */

      if ( ev_bitmap & EC.events )    /* Er der nogle brugbare events? */
	/* løber events igennem; finder mindste først */
	for (i = MAXEVENTNO; i >= 0; i--)
	  if ( (ev_bitmap << i) & (EC.events << i) )
	    {
	      res = MAXEVENTNO - i;
	      fqe = DequeueFQ( EC.msgQ[res] );

	      /* speciel opførsel hvis der er tale om timeout event */
	      if (EC.timer_active && (timeout == (1 << res)))
		{
		  /* hent det specielle timeout element og sæt
		     _last_timeout_id */
		  te = (TimeoutElement) ValueOfFQE( fqe );

		  LOCK( &_last_tid_lock, Wait );
		  _last_timeout_id = te->id;
		  UNLOCK( &_last_tid_lock, Wait );

		  result->msg      = te->msg;
		  result->timer_id = te->id;

		  free( te );
		}
	      else
		/* ellers skal informationen blot videre gives */
		result->msg = ValueOfFQE( fqe );

	      DeleteFQE( fqe );

	      if ( EmptyFQ( EC.msgQ[res] ) )
		EC.events &= ~(1 << res);            /* begivenheden fjernes */
	      found = YES;
          break;
	    }
    }
  UNLOCK( &EC.lock, Wait );
  result->type = (1 << res);
  return result->type;                                 /* eventen returneres */
}
