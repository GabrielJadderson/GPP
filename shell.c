/* 	$Id: shell.c,v 1.7 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVESE: Brugerfunktioner starter op gennem denne funktion.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: shell.c,v 1.7 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern StateControlStruct  SC;           /* Systemets tilstand */

extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern pthread_key_t       id;           /* til identifikation af threaden */


void *shell(void *arg)
{
  char msg[50];

  /*
   * threaden 'husker' sit eget id.
   */
  pthread_setspecific( id, malloc( sizeof( void * )) );
  *((intptr_t *)pthread_getspecific( id )) = (intptr_t)arg;

  /*
   * Venter på GO fra hovedprogrammet. Det er smart at benytte sig af signaler
   * for så undgås et 'busy-wait' loop for at vente på et globalt flag skal
   * blive sat. 
   */
  LOCK( &SC.systemlock, shell );
  pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, shell );

  sprintf(msg, "Starting process (%s)\n", GC.ThArr[(intptr_t)arg]->name );

  if (GC.xterm)
    printf( "%s", msg );

  LOGINFO( msg, strlen(msg) );

  /* Starter brugerfunktionen */
  (GC.ThArr[(intptr_t)arg]->process)();

  /* standser hvis alle brugerprocessere har afsluttet shell */
  LOCK( &SC.systemlock, shell );
  if ( --SC.threads_alive == 0 )
    {
      UNLOCK( &SC.systemlock, shell );
      Stop();
    }
  else
    UNLOCK( &SC.systemlock, shell );
  
  /* I tilfælde af at brugerfunktionen ikke kører uendeligt */
  pthread_exit(NULL);
  return NULL;
}
