/* 	$Id: Signal.c,v 1.4 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Sender signal (event) til alle threads der venter. Hæfter en pointer
 * til en besked (msg) til denne event.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: Signal.c,v 1.4 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <errno.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern EventControlStruct EC;           /* Kontrolstruktur til styring af events*/
extern StateControlStruct SC;           /* Systemets tilstand */

extern log_type           LogStyle;     /* hvordan skal systemet føre log? */

void Signal(long int event, void *msg)
{  
  register int i;
  FifoQueueEntry fqe = NewFQE( msg );
 
  /* undersøger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( Signal );

  LOCK( &EC.lock, Signal );
  
  /* bit'en i bitmønmstret sættes */
  EC.events |= event;

  /* finder index til fifokø-array */
  for (i = 0; i < MAXEVENTNO; i++)
    if ((event >> i) & 1)
      break;

  EnqueueFQ( fqe, EC.msgQ[i] );

  UNLOCK( &EC.lock, Signal );
  
  /* Der sendes et signal til ALLE der venter på EC.signal */
  if (pthread_cond_broadcast( &EC.signal ))
    perror("broadcasting signal in Signal");
  
  return;
}
