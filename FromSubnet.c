/* 	$Id: FromSubnet.c,v 1.5 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Henter data fra input-bufferen.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: FromSubnet.c,v 1.5 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* Samlet kontrolstruktur */
extern StateControlStruct  SC;           /* Systemets tilstand */
extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern int                 ThisStation;  /* nummeret på denne station */

int FromSubnet(int *source, int *dest, char *buffer, int *length)
{
  int res, msglen;
  FifoQueueEntry e;
  BufStr inbuf;
  char msg[BUFFERSIZE + 30];

  /* undersøger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( FromSubnet );

  LOCK( &GC.InData.lock, FromSubnet );           /* lås den kritiske region */
  
  if (GC.InData.size == 0)
    { /* Funktionen er kaldt uden der var noget data at komme efter         */
      res = FAIL;
      UNLOCK( &GC.InData.lock, FromSubnet );  /* lås den kritiske region op */
    }
  else
    {
      e = DequeueFQ( GC.InData.DataQ );

      if (--GC.InData.size == 0)                 /* er der mere i datakøen? */
	res = SUCCES;
      else
	res = MORE;

      UNLOCK( &GC.InData.lock, FromSubnet );  /* lås den kritiske region op */

      /* Der 'castes' til en pointer til hukommelses elememtets data */ 
      inbuf = (BufStr)ValueOfFQE( e ); 
      DeleteFQE( e );

      /* Henter data */
      *source = inbuf->station;
      *dest   = ThisStation;
      memcopy(buffer, (char *)inbuf->data, inbuf->size);
      *length = inbuf->size;

      /* Data kopieres til log-bufferen */
      sprintf(msg, "(%d) queued. FromSubnet: ", GC.InData.size );
      msglen = strlen(msg);
      memcopy( (char *)msg + msglen, (char *)inbuf->data,  inbuf->size );
      msg[msglen + inbuf->size] = 10; /* linieskift                */ 
      LOGINFO( msg , (msglen + inbuf->size + 1));

      GC.StationStat.frame_delv++;
      free( inbuf );
    }
  return res;
}
