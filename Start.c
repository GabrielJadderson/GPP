/* 	$Id: Start.c,v 1.14 1999/09/02 21:48:39 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Starter simuleringen af et netværk. Funktionen sætter stationens
 * socket op, sætter signalhandleren op og starter stationens threads op - både
 * systemthreads og brugerthreads.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: Start.c,v 1.14 1999/09/02 21:48:39 dim Exp $";
*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
extern EventControlStruct  EC;           /* Kontrolstruktur til events */
extern StateControlStruct  SC;           /* systemets tilstand */
extern pthread_key_t       id;           /* thread specifikt data (til thread-id) */

extern char               *StationName;  /* pointer til argv[0] */
extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern int                 ThisStation;  /* nummeret på stationen (fra arg. listen) */

void Start(void)
{
  register intptr_t i, tempsock;
  void *thstatus;
  FifoQueueEntry e;
  char msg[60];
  struct sockaddr_un tempname;
  pthread_attr_t attr;

  /* 
   *---------------------------------------------------------------------------
   * Er denne station aktiv? GC.FQ  bliver initialiseret ved første kald til
   * Activate.
   */
  if (GC.FQ == NULL)
    {
      /* fortæl hovedprogrammet, at der ikke er noget at komme efter her */

      tempsock = socket( AF_UNIX, SOCK_DGRAM, 0);
      if (tempsock < 0)
	perror("Opening socket");

      tempname.sun_family = AF_UNIX;
      sprintf(tempname.sun_path, "Station_0_Socket");
      sprintf(msg, "%d NONE %d", ThisStation, (int)getpid());

      if ( sendto( tempsock, msg, strlen(msg), 0,
		   (struct sockaddr *)&tempname, sizeof(struct sockaddr_un)) < 0 )
	perror("Sending (NONE active)");

      exit(0);
    }
  /*
   *****************************************************************************
   *
   * Stationer er aktiv. Inden de enkelte threads startes op, så er der nogle
   * variable og nogle strukturer der skal sættes op.
   *
   *
   *----------------------------------------------------------------------------
   * Initialiser EventControl:
   */
  EC.events       = NONE;
  EC.timer_active = NO;
  if (pthread_mutex_init( &EC.lock, NULL))
    perror("initializing EC.lock mutex");

  if (pthread_cond_init( &EC.signal, NULL))
    perror("initializing EC.signal cond");

  for (i = 0; i <= MAXEVENTNO; i++)
    EC.msgQ[i] = InitializeFQ();

  /*
   *----------------------------------------------------------------------------
   * signalhandler for program fejl sættes op!
   */
  signal(SIGILL,  signalhandler);     /* for 'illegal instruction'         */
  signal(SIGFPE,  signalhandler);     /* for 'arithmetic exception'        */
  signal(SIGBUS,  signalhandler);     /* for 'bus error'                   */
  signal(SIGSEGV, signalhandler);     /* for 'segmentation fault'          */

  /*
   *---------------------------------------------------------------------------
   * der allokeres plads til id-nøglen, så hver thread kan få sit eget id.
   */
  if (pthread_key_create( &id, free ))
    perror("creating id-key");

  /*
   *---------------------------------------------------------------------------
   * Der allokeres plads til informationer om alle sockets, og der knyttes et
   * navn til alle sockets.
   *
   *
   * De sockets der skal bruges til at kommunikere med andre stationer sættes
   * op.
   */
  GC.SocketArr  = (SocketControl) malloc ( NUMOFSOCKETS 
					   * sizeof( SocketControlStruct ));

  GC.CtrlSocket = (SocketControl) malloc ( NUMOFSOCKETS
					   * sizeof( SocketControlStruct ));

  for (i = 0; i < NUMOFSOCKETS; i++)
    {
      GC.SocketArr[i].sock             = socket( AF_UNIX, SOCK_DGRAM, 0 );
      GC.SocketArr[i].name.sun_family  = AF_UNIX;
      sprintf( GC.SocketArr[i].name.sun_path, "Station_%d_Socket", (int)i );

      GC.CtrlSocket[i].sock            = socket( AF_UNIX, SOCK_DGRAM, 0 );
      GC.CtrlSocket[i].name.sun_family = AF_UNIX;
      sprintf( GC.CtrlSocket[i].name.sun_path, "CtrlStat_%d_Socket", (int)i );

      if (GC.SocketArr[i].sock < 0)
	{
	  sprintf(msg, "Opening data-socket for (Station %d)", ThisStation);
	  perror(msg);
	}

      if (GC.CtrlSocket[i].sock < 0)
	{
	  sprintf(msg, "Opening ctrl-socket for (Station %d)", ThisStation);
	  perror(msg);
	} 
    }

  /*
   * Stationens egne sockets sættes op. Dvs. den socket, der skal modtage data,
   * og den socket der skal modtage information fra 'network'.
   */
  /* Data socket: */
  if (bind( GC.SocketArr[ThisStation].sock,
	    (struct sockaddr *)&GC.SocketArr[ThisStation].name, 
	    sizeof(struct sockaddr_un)))
    {
      /*
       * Navnet var allerede bundet til en socket, det prøves at fjerne denne
       * binding.
       */
      if (errno == EADDRINUSE)
	{
	  unlink( GC.SocketArr[ThisStation].name.sun_path );
	  /* der prøves atter at knytte navn til socket */
	  if (bind( GC.SocketArr[ThisStation].sock,
		    (struct sockaddr *)&GC.SocketArr[ThisStation].name, 
		    sizeof(struct sockaddr_un)))
	    perror("Binding name to own socket, unable to unlink old name");
	}
      else
	perror("Binding name to own socket");
    }


  /* Kontrol informations socket: */
  if (bind( GC.CtrlSocket[ThisStation].sock,
	    (struct sockaddr *)&GC.CtrlSocket[ThisStation].name, 
	    sizeof(struct sockaddr_un)))
    {
      /*
       * Navnet var allerede bundet til en socket, det prøves at fjerne denne
       * binding.
       */
      if (errno == EADDRINUSE)
	{
	  unlink( GC.CtrlSocket[ThisStation].name.sun_path );
	  /* der prøves atter at knytte navn til socket */
	  if (bind( GC.CtrlSocket[ThisStation].sock,
		    (struct sockaddr *)&GC.CtrlSocket[ThisStation].name, 
		    sizeof(struct sockaddr_un)))
	    perror("Binding name to ctrl-socket, unable to unlink old name");
	}
      else
	perror("Binding name to ctrl-socket");
    }

  /*
   *---------------------------------------------------------------------------
   * Hvis der ønskes at systemet skal føre log selvstændigt for denne
   * station, så initialiseres logbufferen.
   */
  if (LogStyle == separate)
    {
      sprintf(msg, "%s.system", StationName );

      /*
       * Dette er lidt et 'hack'. Det er for at fjerne './' fra starten af
       * programnavnet. './' bliver sat på af 'network' for at være sikker på at
       * programmet startes fra nuværende 'directory'.
       */
      for( i = 2; msg[i] != 0; i++)
	msg[i-2] = msg[i];

      msg[i-2] = 0;
      GC.logbuffer = InitializeLB( msg );
    } 

  /*
   *---------------------------------------------------------------------------
   * Brugerfunktionerne gøres til threads, og stationens egne threads startes
   * op. Disse funktioner startes ikke direkte op, men via shell.
   */

  /* Der allokeres plads til alle threads. */
  GC.ThArr = (ThreadControl *) malloc ((GC.NumOfThreads + SYSTEMTHREADS)
					 * sizeof( ThreadControl ));
  /*
   * Attributter sættes op.
   */

  if (pthread_attr_init( &attr ))
    perror("creating stack attribut to threads");

  if (pthread_attr_setstacksize( &attr, STACKSIZE ))
    perror("setting stacksize for threads");

  if (pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED ))
    perror("setting detachstate");

  /* De enkelte threads startes */
  for (i = 0; i < GC.NumOfThreads; i++)
    {
      /* Der hentes en process fra fifo-køen */
      e =  DequeueFQ( GC.FQ );       /* for ikke at "grisse" med hukommelsen */
      GC.ThArr[i] = (ThreadControl) ValueOfFQE(e);
      DeleteFQE(e);                  /* ... så kan e nemlig de-allokeres!    */

      if (pthread_create( &GC.ThArr[i]->th, &attr, shell, (void *)i))
	printf("Error creating thread; thread no. %d\n", (int)i);
      else
	SC.threads_alive++;          /* endnu en 'shell-thread' startet op   */
    }
  DeleteFQ( GC.FQ );               /* Den sidste rest af fifokøen forsvinder */

  /*
   * Stationens thread der arbejder med sockets sættes op...
   * `i' har værdien 'GC.NumOfThreads' efter ovenfor stående løkke er færdig,
   * og dette er det næste brugbare index
   */

  /* Threaden til flow kontrol startes */
  GC.ThArr[i] = (ThreadControl) malloc (sizeof(ThreadControlStruct));

  if (pthread_create( &GC.ThArr[i]->th, &attr, flow, (void *)i))
    printf("Error creating flow; thread no. %d\n", (int)i);

  /* Threaden til behandling af 'fejl' startes */
  GC.ThArr[i] = (ThreadControl) malloc (sizeof(ThreadControlStruct));

  if (pthread_create( &GC.ThArr[i]->th, &attr, errortimer, (void *)i))
    printf("Error creating errortimer; thread no. %d\n", (int)i);

  /* Threaden til behandling af 'timeouts' startes */
  GC.ThArr[++i] = (ThreadControl) malloc (sizeof(ThreadControlStruct));

  if (pthread_create( &GC.ThArr[i]->th, &attr, timer, (void *)i))
    printf("Error creating timer; thread no. %d\n", (int)i);

  /* Thread til modtagelse af data via socket sættes op */
  GC.ThArr[++i] = (ThreadControl) malloc (sizeof(ThreadControlStruct));

  if (pthread_create( &GC.ThArr[i]->th, &attr, receiver, (void *)i))
    printf("Error creating receiver; thread no. %d\n", (int)i);

  /* Thread til modtagelse af kontrol info via socket s�ttes op */
  GC.ThArr[++i] = (ThreadControl) malloc (sizeof(ThreadControlStruct));

  /* BEMÆRK! denne thread startes med standard attributter */
  if (pthread_create( &GC.ThArr[i]->th, NULL, control, (void *)i))
    printf("Error creating control; thread no. %d\n", (int)i);

  /* Attributterne de-allokeres */
  if (pthread_attr_destroy( &attr ))
    perror("deleting thread attribut");

  /*
   * Der udføres kun en 'join' ved denne thread. Dette virker som 'wait' ved
   * fork. Threaden terminerer ikke, det er blot for at undgå et 'busy-wait loop'
   */
  if (pthread_join( GC.ThArr[i]->th, &thstatus))
    printf("Fejl ved join af kontrol thread (no %d, station %d).\n",
	   (int)i, ThisStation);

  /* Hvis programmet når hertil, så er der sket en fejl! */
    printf("ERROR! Station %d - abnormal program termination\n", ThisStation); 
  sleep(10);
  exit(1);
}
