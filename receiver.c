/* 	$Id: receiver.c,v 1.13 1999/09/04 12:05:18 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Denne funktion lytter på stationen socket og opsamler
 * data. Det er også denne funktion der afgør om data skal forsinkes, smides
 * væk, eller leveres direkte til inddatabufferen.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: receiver.c,v 1.13 1999/09/04 12:05:18 dim Exp $";
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern StateControlStruct  SC;           /* systemets tilstand */
extern FlowControl         FC;           /* flow control */

extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern int                 ThisStation;  /* stationens nummer */
extern pthread_key_t       id;           /* til identifikation af threaden */

                        /* (debug) for at kunne tælle antal "Round Robin's" */
#ifdef DEBUG_CONTSWP
#define DEBUG_CONTSW
#endif

#ifdef DEBUG_CONTSW
extern int                  context_switch;
#endif

void *receiver(void *arg)
{
  int ready = YES, msglen;
  char msg[BUFCTRLSIZE + BUFFERSIZE + 50];
  BufStr buffer, tempbuffer;

  /*
   * ---------------------------------------------------------------------------
   * threaden 'husker' sit eget id og navn.
   */
  pthread_setspecific( id, malloc( sizeof( void * )) );
  *((intptr_t *)pthread_getspecific( id )) = (intptr_t)arg;

  GC.ThArr[(intptr_t)arg]->name = strcpy( (char *) malloc (sizeof(char)* 22), 
 				     "(SYSTEM) receiver");

  /* venter på et GO fra hoved programmet */
  LOCK( &SC.systemlock, receiver );
  pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, receiver );

  /*
   *---------------------------------------------------------------------------
   * Funktionen sætter sig nu til at vente på at der dukker noget data op...
   */

  while(1)
    {
      /* allokerer 'ren' hukommelse */
      buffer  = (BufStr) calloc (sizeof(BufStrStruct), sizeof(char));

      if (read(GC.SocketArr[ThisStation].sock, (char *)buffer,
	       BUFCTRLSIZE + BUFFERSIZE) < 0)
	{
	  perror("Receiving datagram");
	  sleep(1);
	}

      if ( buffer->type == (unsigned char)stop )
	/* så stopper vi! */
	{
	  /* udskriver kun hvis der er noget at udskrive */
	  if (buffer->size > 0) 
	    {
	      sprintf(msg,  "<<%s received.>>\n", buffer->data );
	      LOGINFO( msg, strlen(msg) );
	      
	      if (GC.xterm)
		printf("%s", msg);
	    }

	  usleep(300000);
	  close_station(RECEIVER);
	}

      /* hvis de er ved at blive lukket, så modtages der ikke mere data! */
      LOCK( &SC.systemlock, receiver );
      if ( SC.ready != YES )
	ready = NO;
      UNLOCK( &SC.systemlock, receiver );
      
      if ( (ready && buffer->type == (unsigned char)data ))
	{
	  /* Opdaterer sekvensnummer modtaget */
	  LOCK( &FC[buffer->station].lock, receiver );
	  FC[buffer->station].last_recv_seq = buffer->seq;
	  UNLOCK( &FC[buffer->station].lock, receiver );
	  
	  /* Hvad skal der nu ske med pakken */
	  switch (transmit_error())
	    {
	    case none:                                          /* ingen fejl! */
	      LOCK( &GC.InData.lock, receiver );
	      sprintf( msg, "(%d) - queued. recv(#%d,%d): ",
		       GC.InData.size, buffer->seq, buffer->station);
	      UNLOCK( &GC.InData.lock, receiver );

	      msglen = strlen(msg);
	      memcopy( (char *)msg + msglen, (char *)buffer->data, buffer->size );
	      msg[msglen + buffer->size] = 10   ;            /* linieskift      */
	      LOGINFO( msg, (msglen + buffer->size + 1 ));
	      transfer_frame( buffer );
	      break;
	    
	    case loose:                                  /* glem alt om pakken  */
	      GC.StationStat.frame_lost++;               /* ...noteres!         */
	      sprintf( msg, "lost(#%d,%d): ", buffer->seq, buffer->station);
	      msglen = strlen(msg);
	      memcopy( (char *)msg + msglen, (char *)buffer->data, buffer->size );
	      msg[msglen + buffer->size] = 10;               /* linieskift      */
	      LOGINFO( msg, (msglen + buffer->size + 1 ));
	      free( buffer );
	      break;
	    
	    case delay:                                    /* pakken forsinkes */
	      sprintf( msg, "delay(#%d,%d): ", buffer->seq, buffer->station);
	      msglen = strlen(msg);
	      memcopy( (char *)msg + msglen, (char *)buffer->data, buffer->size );
	      msg[msglen + buffer->size] = 10;              /* linieskift      */
	      LOGINFO( msg, (msglen + buffer->size + 1 ));
	      delay_frame( buffer, MULTIPLELIMIT );

#ifdef DEBUG_ERR_DQ
	      LOCK( &GC.DelayData.errorlock, receiver );
	      PrintPQ( GC.DelayData.ErrorDataQ, PrintBuf );
	      UNLOCK( &GC.DelayData.errorlock, receiver );
#endif
	      break;
	    
	    case multiple:                    /* en pakke nu og en igen senere */
	      GC.StationStat.frame_dupl++;              /* ...noteres!         */
	      LOCK( &GC.InData.lock, receiver );
	      sprintf( msg, "(%d) - queued. multiple(#%d,%d): ", 
		       GC.InData.size, buffer->seq, buffer->station );
	      UNLOCK( &GC.InData.lock, receive );

	      msglen = strlen(msg);
	      memcopy( (char *)msg + msglen, (char *)buffer->data, buffer->size );
	      msg[msglen + buffer->size] = 10;              /* linieskift      */
	      LOGINFO( msg, (msglen + buffer->size + 1 ));

	      /* kopierer indholdet af elementet */
	      tempbuffer = memcopy((BufStr) calloc (sizeof(BufStrStruct),
						    sizeof(char)),
				   (char *)buffer,
				   sizeof(BufStrStruct));
	      
              transfer_frame( buffer );	      
	      /*
	       * I transmit_error sørges for at der kun springes til multiple
	       * hvis MULTIPLELIMIT er større end nul
	       */
	      delay_frame( tempbuffer, MULTIPLELIMIT - 1 );
	      break;
	    }
	}
    }
#if !defined (__sun)
	  return NULL;
#endif
}
