/* 	$Id: re_delay_frame.c,v 1.5 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Funktionen tager en pakke fra fejldatakøen. Denne leveres
 * både som nyankommet data, samt genindsættes i fejldatakøen.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: re_delay_frame.c,v 1.5 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */

/*
 * Genindsætter data i fejl-data bufferen (som dublet)
 */
inline void re_delay_frame(PriQueueEntry e)
{
  PriQueueEntry pqe;
  
  /* der trækkes et tilfældigt tal (til offset af forsinkelse) */
  register int key = ((rand() >> 8) & (DELAYCNTMAX - DELAYCNTMIN))+ DELAYCNTMIN;

  /* multiple_ok tælles en ned, da der dubleres */
  ((DelayDataElement)ValueOfPQE( e ))->multiple_ok--;

  /* dubletten noteres */
  GC.StationStat.frame_dupl++;

  /* læs fejldatakøen */
  LOCK( &GC.DelayData.errorlock, re_delay );

  /* indsætter gen-forsinkelsen */
  key += GC.DelayData.errorcount;
  pqe = NewPQE( key, ValueOfPQE(e));
  InsertPQ( pqe, GC.DelayData.ErrorDataQ );

  UNLOCK( &GC.DelayData.errorlock,re_delay );

  pthread_cond_signal( &GC.DelayData.errorsignal );
}
