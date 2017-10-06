/* 	$Id: transfer_frame.c,v 1.6 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Funktionen indsætter en hukommelsescelle i inddatakøen. Denne
 * hukommelsescelle indeholder data der er modtaget via stationens socket (ved
 * kald fra receiver) eller data der stammer fra stationens fejldatakø (ved kald
 * fra errortimer).
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: transfer_frame.c,v 1.6 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern log_type            LogStyle;     /* hvordan skal systemet føre log? */

/*
 * Indsætter data i bufferen.
 */
inline void transfer_frame(BufStr buffer)
{
  char msg[] =  "inbuffer overflow, packet dumped.\n";
  FifoQueueEntry fqe = NewFQE( (void *)buffer );

  /* inddatakøen låses */
  LOCK( &GC.InData.lock, transfer );
  EnqueueFQ( fqe, GC.InData.DataQ );

  /* 
   * checker om der er mere plads i inddatakøen. Hvis ikke, smides det
   * ældste element ud og 'mem_elem' indsættes.
   */
  if (GC.InData.size == INDATALIMIT)
    {
      fqe = DequeueFQ( GC.InData.DataQ );
      UNLOCK( &GC.InData.lock, transfer );
      
      LOGINFO( msg, strlen(msg));
      
      /* Det noteres, at der er slettet en pakke */
      GC.StationStat.frame_eras++;
      
      free( (BufStr)ValueOfFQE( fqe ) );
      DeleteFQE( fqe );
    }
  else 
    {
      GC.InData.size++;
      UNLOCK( &GC.InData.lock, transfer );
    }
  GC.StationStat.frame_recv++;
  Signal( frame_arrival, NULL );
}
