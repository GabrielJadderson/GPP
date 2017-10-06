/* 	$Id: Stop.c,v 1.9 1999/09/02 21:50:19 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Funktion standser simuleringen ved at sende en 'stop'-datapakke til 
 * alle stationers data-socket, inkl. sig selv. Dette gøres fordi 'receiver' har
 * kontrol over socket'en, og det kan ikke lade sig gøre at lukke den så længe der
 * bliver læst fra den.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: Stop.c,v 1.9 1999/09/02 21:50:19 dim Exp $";
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern StateControlStruct  SC;           /* kontrol af systemets tilstand */
extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern int                 ThisStation;  /* nummeret på stationen (fra arg. listen) */

void Stop()
{
  int i, ready;
  BufStrStruct buffer;
  const char msg[] = "<<Sending stop-signal>>\n";
  
  sprintf( (char *)buffer.data, "stop-signal");
  buffer.type    = stop;
  buffer.station = ThisStation;
  buffer.size    = strlen((char *)buffer.data);

  /* undersøger om stationen er klar eller ved at lukke ned */
  LOCK( &SC.systemlock, Stop );
  ready = SC.ready;
  UNLOCK( &SC.systemlock, Stop );

  /* stationen ER ved at lukke ned */
  if (ready != YES)
    pthread_exit(NULL);
  else
    {
      LOGINFO( msg, strlen(msg) );
      if (GC.xterm)  
	printf("%s", msg);
    }

  /* læser socketen før der sendes */
  LOCK( &GC.OwnSocketLock, Stop );
  
  for (i = 0; i <= GC.MaxStation; i++)
    /* sender DIE til alle */
    if (GC.active[i])
      {
      send_stop_label:
	if (sendto( GC.SocketArr[i].sock, (char *)&buffer,
		    BUFCTRLSIZE + buffer.size, 0,
		    (struct sockaddr *)&GC.SocketArr[i].name,
		    sizeof(struct sockaddr_un)) < 0 )
	  {
	    /* Det går for hurtigt, til at maskinen kan følge med */
	    if (errno == ENOBUFS)
	      {
		usleep(30000); /* 3/100 sek. */
		goto send_stop_label;
	      }
	    
	    if (errno != ENOENT)   /* to 'stop signaler' har krydset hinanden */
	      perror("Sending Stop signal");
	  }
      }
  UNLOCK( &GC.OwnSocketLock, Stop );  
  sleep(1);
  pthread_exit(NULL);
}
