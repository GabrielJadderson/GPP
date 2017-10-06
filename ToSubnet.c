/* 	$Id: ToSubnet.c,v 1.8 1999/09/04 11:31:01 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Sender data over subnettet.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: ToSubnet.c,v 1.8 1999/09/04 11:31:01 dim Exp $";
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"


extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern StateControlStruct  SC;           /* systemets tilstand */
extern FlowControl         FC;           /* flow control */

extern log_type            LogStyle;     /* hvordan skal systemet føre log? */

int ToSubnet(int source, int dest, char *buffer, int length)
{
  int res = FAIL;  /* er modtagerstationen ikke aktiv, så er FAIL resultatet*/
  int localerrno;  /* for at mindske fejlrisikoen på `errno' ved to fejl efter
		    * tæt efter hinanden i forskellige threads */
  int msglen;
  BufStrStruct outbuf;
  char msg[BUFCTRLSIZE + BUFFERSIZE + 20];
  char errormsg[] = "ToSubnet - illegal station number\n"; 

  /* undersøger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( ToSubnet );

  /* Undersøger om det er legale argumenter */
  if ((source < 1) || (source > GC.MaxStation) ||
      (dest < 1) || (dest > GC.MaxStation))
    {
      LOGINFO( errormsg, strlen(errormsg) );
      printf( "%s", errormsg );
      return FAIL; /* dvs. illegale afsender/modtager station */
    }

  outbuf.type    = data;
  outbuf.station = source;

  /* sæt str. til 'length' eller 'BUFFERSIZE', alt efter hvad der er mindst */
  outbuf.size    = ( length < BUFFERSIZE ? length : BUFFERSIZE );

  /* kopiering af data til bufferen */
  memcopy( (char *)outbuf.data, buffer, outbuf.size );

  /* Hvis stationen er meldt aktiv, så sendes der til den */
  if ( GC.active[dest] ) 
    {
      /* Der checkes om det er lovligt (sekvensnummer) at sende til stationen */
      LOCK( &FC[dest].lock, ToSubnet );

      if (FC[dest].next_send_seq > FC[dest].upper_limit)
	/* ikke lovligt, vent på signal. Husk: mutex slippes automatisk ved
	   wait, og gribes igen når signalet modtages. */
	pthread_cond_wait( &FC[dest].signal, &FC[dest].lock );

      /* sæt sekvensnummer, og opdater next_send_seq */
      outbuf.seq = FC[dest].next_send_seq++;
      UNLOCK( &FC[dest].lock, ToSubnet );

      /* Data kopieres til log-bufferen */
      sprintf( msg, "sending(#%d,%d): ", outbuf.seq, dest );
      msglen = strlen(msg);
      memcopy( (char *)msg + msglen, buffer, outbuf.size );
      msg[msglen + outbuf.size]     = 10; /* linieskift     */
      LOGINFO( msg, (msglen + outbuf.size +1) );

      /* låser socketen for afsendelse */
      LOCK( &GC.OwnSocketLock, ToSubnet );
      
      /* Ja, jeg ved det godt - men det er det nemmeste... */
    send_label:
      
      if (sendto( GC.SocketArr[dest].sock, (char *)&outbuf,
		  BUFCTRLSIZE + outbuf.size,
		  0,(struct sockaddr *)&GC.SocketArr[dest].name,
		  sizeof(struct sockaddr_un)) < 0 )
	{
      	  localerrno = errno;

	  /* Det går for hurtigt, til at maskinen kan følge med */
	  if (localerrno == ENOBUFS)
	    {
	      usleep(10000); /* 1/100 sek. */

	      /* tving et processkift */
	      sched_yield();

	      goto send_label;
	    }

	  /*
	   * Her skulle der være en SYSTEMOK  for at checke om systemet stadig
	   * er kørende, men for at undgå deadlocks, så benyttes en 'trylock'
	   * i stedet, så der kun checkes for kørende system, hvis systemets
	   * 'mutex' ikke er blokeret, for så vigtigt er SYSTEMOK ikke!
	   */
	  if( pthread_mutex_trylock( &SC.systemlock ))
	    perror("ToSubnet locking &SC.systemlock");  /* Fejl */
	  else
	    {                                           /* mutex var tilgængelig */
	      if (SC.ready != YES )
		pthread_cond_wait( &SC.systemsignal, &SC.systemlock );

	      if( pthread_mutex_unlock( &SC.systemlock  ))
		perror("ToSubnet un-locking &SC.systemlock");
	    } 

	  perror("sending message");
	  res = FAIL;

	  /* modtager socketen er forsvundet */
	  if (localerrno == ENOENT)
	    {
	      sprintf( msg, "No contact to station %d\n", dest );
	      printf( "%s", msg );
	      GC.active[dest] = NO; /* Det er faktisk en kritisk region. Der er 
				     * lås på stationens socket.*/

	      UNLOCK( &GC.OwnSocketLock, ToSubnet );
	      LOGINFO( msg, strlen(msg) );
	      Stop();
	    }
	}
      else
	{
	  GC.StationStat.frame_sent++;
	  res = SUCCES;
	}

      UNLOCK( &GC.OwnSocketLock, ToSubnet );
    }
  else
    {
      /* modtager stationen er ikke meldt aktiv */
      sprintf( msg, "Station %d is not active.\n", dest );
      printf("%s", msg);
      LOGINFO( msg, strlen(msg) );
    }
       
  return res;
}
