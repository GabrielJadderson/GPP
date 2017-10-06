/* 	$Id: signalhandler.c,v 1.9 1999/09/02 22:07:27 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Signalhandler til SIGILL, SIGFPE, SIGBUS, SIGSEGV. 
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: signalhandler.c,v 1.9 1999/09/02 22:07:27 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"


extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern StateControlStruct  SC;           /* Systemets tilstand */

extern int                 ThisStation;  /* stationens nummer */
extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern pthread_key_t       id;           /* til identifikation af threaden */

/*
 * Fejlbehandlingsfunktion. For at få at vide hvilken thread fejlen opstod i,
 * så er det nødvendigt med denne signalhandler. Udover at skrive hvilken
 * thread fejlen opstod i, så blokerer denne funktion også for videre
 * programudførsel, samt hindre xterm-vinduet i at forsvinde før man kan nå
 * at løse fejlen.
 */ 
void signalhandler(int sig)
{
  char msg[50], logmsg[120];
  int i, ready;
  intptr_t thid;
  BufStrStruct buffer;

  /* ignorer flere signaler af samme slags. */
  signal(sig, SIG_IGN);

  /* i hvilken thread opstod fejlen? */
  thid = *((intptr_t *)pthread_getspecific( id ));

  /*
   * Giver besked om hvilket signal der er fanget.
   */
  switch (sig)
    {
    case SIGILL:
      strcpy(msg, "(illegal instruction) SIGILL");
      break;

    case SIGFPE:
      strcpy(msg, "(floating point exception) SIGFPE");
      break;

    case SIGBUS:
      strcpy(msg, "(bus error) SIGBUS");
      break;

    case SIGSEGV:
      strcpy(msg, "(segmentation fault) SIGSEGV");
      break;
    }
  
  sprintf( logmsg, ">> error in process: %s\n", GC.ThArr[thid]->name);
  LOGINFO( logmsg, strlen(logmsg) );

  sprintf( logmsg, ">> %s signal caught.\n", msg );
  LOGINFO( logmsg, strlen(logmsg) );

  /*
   * gi' de andre stationer besked om at der er sket en fejl
   */
  sprintf( (char *)&buffer.data, "[%s in station %d] - Stop",
	   msg, ThisStation);
  buffer.type    = stop;
  buffer.station = ThisStation;
  buffer.size    = strlen((char *)&buffer.data);

  /* lås socket for afsendelse */
  LOCK( &GC.OwnSocketLock, signalhandler );

  for (i = 0; i <= GC.MaxStation; i++)
    /* sender DIE til alle */
    if ((GC.active[i]) && (i != ThisStation )) 
      if (sendto( GC.SocketArr[i].sock, (char *)&buffer,
		  BUFCTRLSIZE + buffer.size,
		  0, (struct sockaddr *)&GC.SocketArr[i].name,
  		  sizeof(struct sockaddr_un)) < 0 )
  	if (errno != ENOENT)    /* to 'stop signaler' har krydset hinanden */
  	  perror("Sending Stop signal");
  
  UNLOCK( &GC.OwnSocketLock, signalhandler );

  /*
   * for at bremse den øvrige programudførsel
   */
  LOCK( &SC.systemlock, signalhandler );
  ready = SC.ready;
  SC.ready = STOPPING;
  UNLOCK( &SC.systemlock, signalhandler );

  /*
   * Det er ikke sikkert udskrift er muligt, hvis det er en af systemets threads
   * der er gået ned. System-threads har højere id-numre end brugerfunktionerne.
   */
  if (thid < GC.NumOfThreads)
    FlushAllLog();
  
  LOCK( &SC.systemlock, signalhandler );
  SC.ready = NO;
  UNLOCK( &SC.systemlock, signalhandler );

  printf("\n>> error in process: %s\n", GC.ThArr[thid]->name );
  
  if (GC.xterm)
    {
      printf("\a>> %s signal caught.\n>> Press enter to terminate!%s\n",
	     msg, (thid < GC.NumOfThreads? "" : " (may need ^C)"));

      /* resetter ctrl-c */
      signal(SIGINT, SIG_DFL);

      /*
       * for at undgå, at vinduet forsvinder inden man kan nå at se hvad der er sket
       */
      if (ready != NO)
	while ( getchar() != 10 );

      exit(-1);
    }
  else
    {
      printf("\a>> %s signal caught in station %d.\n", msg, ThisStation);
      sleep(1);
      exit(-1);
    }
}
