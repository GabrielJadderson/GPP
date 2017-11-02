/* 	$Id: delay_frame.c,v 1.5 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Funktionen indsætter en hukommelsescelle i fejldatakøen. Denne
 * hukommelsescelle indeholder data der er modtaget via stationens socket.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: delay_frame.c,v 1.5 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */

/*
 * Inds�tter data i fejl-data bufferen
 */
inline void delay_frame( BufStr buffer, int mul_limit )
{
  register int key   = ((rand() >> 8) & (DELAYCNTMAX - DELAYCNTMIN))+ DELAYCNTMIN;
  PriQueueEntry pqe;
  DelayDataElement e = (DelayDataElement) malloc
                       (sizeof(DelayDataElementStruct));

  /* kø elementet sættes op; antal mulige gentagelser, hukommelsescelle */
  e->multiple_ok = ( mul_limit );
  e->data        = buffer;

  /* fejldatakøen læses */
  LOCK( &GC.DelayData.errorlock, delay );

  /* elementet indsættes i fejldatakøen */
  key += GC.DelayData.errorcount;
  pqe = NewPQE( key, (void *)e );
  InsertPQ( pqe, GC.DelayData.ErrorDataQ );

  UNLOCK( &GC.DelayData.errorlock, delay );
  pthread_cond_signal( &GC.DelayData.errorsignal );
}
