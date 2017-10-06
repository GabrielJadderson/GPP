/* 	$Id: ClearEvent.c,v 1.3 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Fjerner et signal fra event køen.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: ClearEvent.c,v 1.3 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern EventControlStruct EC;           /* Kontrolstruktur til styring af events*/
extern StateControlStruct SC;           /* Systemets tilstand */

extern log_type           LogStyle;     /* hvordan skal systemet føre log? */

int ClearEvent(long int event)
{  
  int i, count = 0;
  FifoQueueEntry fqe;
 
  /* undersøger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( ClearEvent );

  LOCK( &EC.lock, ClearEvent );
  
  /* bit'en i bitmønstret fjernes */
  EC.events &= ~event;

  /* finder index til fifokø-array */
  for (i = 0; i < MAXEVENTNO; i++)
    if ((event >> i) & 1)
      break;

  while (!EmptyFQ(EC.msgQ[i]))
    {
      fqe = DequeueFQ( EC.msgQ[i] );
      free( (void *)ValueOfFQE( fqe ) );
      DeleteFQE( fqe );
      count++;
    }

  UNLOCK( &EC.lock, ClearEvent );
  
  /* returnerer antallet af events der blev fjernet */
  return count;
}
