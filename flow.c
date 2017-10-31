/* 	$Id: flow.c,v 1.1 1999/09/02 22:18:32 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Denne funktion startes som selvstændig tråd. Tråden står for
 * flow kontrol. Det fungerer således: når der modtages en pakke fra en anden
 * station, så noteres sekvensnummeret for denne pakke under pågældende station.
 * denne tråd sender så 'credits' til de stationer der modtages data fra. Den
 * kredit der sendes er det sidst registrede sekvensnummer fra pågældende
 * station + vinduesstørrelse. Dette bevirker at der kun er en begrænset mængde
 * data 'på linien' af gangen, da en senderstation kun kan våre foran med en
 * vinduesstårrelse i forhold til modtagerstationen - det er det den har kredit
 * til.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: flow.c,v 1.1 1999/09/02 22:18:32 dim Exp $";
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


void *flow(void *arg)
{
  int i, flag, cur_winsize = WINSIZE;
  BufCtrlStruct buffer;
  int prev_seq[NUMOFSOCKETS];
    
  /*
   * ---------------------------------------------------------------------------
   * threaden 'husker' sit eget id og navn.
   */
  pthread_setspecific( id, malloc( sizeof( void * )) );
  *((intptr_t *)pthread_getspecific( id )) = (intptr_t)arg;

  GC.ThArr[(intptr_t)arg]->name = strcpy( (char *) malloc (sizeof(char)* 22), 
 				     "(SYSTEM) flow");

  /* venter på et GO fra hovedprogrammet */
  LOCK( &SC.systemlock, flow );
  pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, flow );

  /* for at kunne undersøge om der er sket ændringer siden sidst */
  for (i = 1; i < NUMOFSOCKETS;i++) 
    {
      LOCK( &FC[i].lock, flow );
      prev_seq[i] = FC[i].last_recv_seq;
      UNLOCK( &FC[i].lock, flow );
    }

  /*
   *---------------------------------------------------------------------------
   * Der startes en uendelig løkke, hvor der sendes 'credits' til de andre
   * stationer.
   */

  buffer.type    = credit;
  buffer.station = ThisStation;

  while(1)
    {
      /* undersøger om stationen er ved at lukke */
      SYSTEMOK( flow );

      /* check flow.... */
      for (i = 1; i < NUMOFSOCKETS; i++)
	{
	  /* sendet til de aktive stationer (... dog ikke sig selv ...) */
	  if (( i != ThisStation ) && GC.active[i] )
	    {
	      LOCK( &FC[i].lock, flow );
	      /* er der sket �ndringer? */
	      if (prev_seq[i] != FC[i].last_recv_seq)
		{
		  prev_seq[i]  = FC[i].last_recv_seq;
		  buffer.limit = FC[i].last_recv_seq + cur_winsize;
		  flag = TRUE;                   /* der skal sendes kredit */
		}
	      else
		{
		  flag = FALSE;             /* der skal IKKE sendes kredit */
		}
	      UNLOCK( &FC[i].lock, flow );
	 
	      if (flag)
		{
		  /* endnu et fy, skamme goto-label.... */
		  LOCK( &GC.OwnSocketLock, flow);
		send_credit_label:
		  
		  /* sender ny grænse */
		  if (sendto( GC.CtrlSocket[i].sock, (char *)&buffer,
			      sizeof(BufCtrlStruct), 0,
			      (struct sockaddr *)&GC.CtrlSocket[i].name,
			      sizeof(struct sockaddr_un)) < 0 )
		    {
		      /* Det går for hurtigt, til at maskinen kan følge med */
		      if (errno == ENOBUFS)
			{
			  usleep(30000); /* 3/100 sek. */
			  goto send_credit_label;
			}
		      
		      perror("Sending credit");
		    }
		  
		  UNLOCK( &GC.OwnSocketLock, flow);
		}
	    }
	} /* alle statione er løbet igennem */
      
      sched_yield(); /* skift */
    }
#if !defined (__sun)
	  return NULL;
#endif
}
