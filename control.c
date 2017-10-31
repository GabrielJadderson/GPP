/* 	$Id: control.c,v 1.2 1999/09/02 22:04:40 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Denne funktion lytter på stationen kontrol socket. Desuden
 * foregår der ydeligere initialisering af stationen, udover hvad der er i
 * Start. Det er receive der giver signal til de øvrige threads når alt
 * er klart.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: control.c,v 1.2 1999/09/02 22:04:40 dim Exp $";
*/
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>

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


void *control(void *arg)
{
  struct timeval tv;
  struct timezone tz;
  int i;
  char msg[BUFCTRLSIZE + BUFFERSIZE + 50];
  char setupbuffer[SETUPINFOSIZE];
  BufCtrlStruct buffer;

  /*
   * ---------------------------------------------------------------------------
   * threaden 'husker' sit eget id og navn.
   */
  pthread_setspecific( id, malloc( sizeof( void * )) );
  *((intptr_t *)pthread_getspecific( id )) = (intptr_t)arg;

  GC.ThArr[(intptr_t)arg]->name = strcpy( (char *) malloc (sizeof(char)* 22), 
 				     "(SYSTEM) control");

  /*
   *---------------------------------------------------------------------------
   * Initialiserer køerne ;-) i InData og DelayData
   */
  GC.InData.DataQ         = InitializeFQ();
  GC.DelayData.ErrorDataQ = InitializePQ();

  
  /*
   *---------------------------------------------------------------------------
   * Sætter array op til flow control. Der er et entry til hver socket, også
   * selvom index '0' ikke skal bruges. Dette for at stationsnummer kan bruges
   * som index. (for nemheds/læseligheds skyld)
   */
  FC = (FlowControl) malloc (NUMOFSOCKETS * sizeof(FlowControlStruct));
  
  for (i = 0; i < NUMOFSOCKETS; i++)
    {
      FC[i].next_send_seq = 1;                        /* n�ste der skal sendes */
      FC[i].upper_limit   = WINSIZE; /* �vre gr�nse, afsendelse - udgangspunkt */
      FC[i].last_recv_seq = 0;                               /* sidst modtaget */

      if (pthread_mutex_init(&FC[i].lock, NULL)) 
	{
	  perror("control - initializing mutex");
	}

      if (pthread_cond_init(&FC[i].signal, NULL))
	{
	  perror("control - initializing condition");
	}
    }

  /*
   *---------------------------------------------------------------------------
   * Nu er modtager-threaden kørende, så der sendes et 'ready' til
   * hovedprogrammet.
   */

  printf("[Station %d ready]", ThisStation);
  fflush(stdout);

  sprintf(setupbuffer, "%d READY %d", ThisStation, (int)getpid());
  if (sendto( GC.SocketArr[0].sock, setupbuffer, SETUPINFOSIZE, 0, 
	      (struct sockaddr *)&GC.SocketArr[0].name,
	      sizeof(struct sockaddr_un)) < 0 )
    perror("Sending ready message");
  
  /*
   *---------------------------------------------------------------------------
   * Der ventes på GO fra hovedprogrammet. GO kommer i form af et array, der
   * indeholder information om hvor mange stationer der er, hvilke stationer der 
   * er aktive, fejlfrekvensen og om stationen er startet i et x-term vindue.
   *
   * Arrayet er formateret sledes:
   * {station startet i x-term vindue ja/nej (1/0),
   *  antal stationer,
   *  station 1 -  aktiv/ikke aktiv,
   *  ... ,
   *  antal 1000'er i fejlfrekvens.
   *  antal 100'er  i fejlfrekvens.
   *  antal 10'er   i fejlfrekvens.
   *  antal 1'er    i fejlfrekvens.}
   *
   *
   * 'read' blokerer indtil der modtages en pakke. Der lyttes på 'kontrol' socket -
   * ikke på 'data' socket.
   */

  if (read(GC.CtrlSocket[ThisStation].sock, setupbuffer, SETUPINFOSIZE) < 0)
    perror("Receiving GO message");
 
  /* Er stationen startet i et x-term vindue */
  if ((int)setupbuffer[0] == 1)
    GC.xterm = YES;
  else
    GC.xterm = NO;

  /* Information om antallet af �vrige stationer gemmes */
  GC.MaxStation = (int)setupbuffer[1];      /* indeholder ANTAL stationer i alt */

  GC.active = (int *) calloc (sizeof(int) * (MAXSTATION + 1), sizeof(int));
  GC.active[0] = 1;                         /* hovedprogrammet er altid aktivt! */

  /* Informationer om hvilke stationer der er aktive gemmes */
  for (i = 1; i <= GC.MaxStation; i++)
    GC.active[i] = (int)setupbuffer[i+1];

  /* fejlfrekvensen gemmes; */
  GC.ErrorFreq  = setupbuffer[++i] * 1000;
  GC.ErrorFreq += setupbuffer[++i] * 100;
  GC.ErrorFreq += setupbuffer[++i] * 10;
  GC.ErrorFreq += setupbuffer[++i];


  /*
   *****************************************************************************
   * Opsætningen er slut...
   *
   *
   * Nu må der gives los... ( alle bruger funktioner venter i 'shell' på dette
   * signal.
   */

  if (GC.xterm) 
    printf("\nGO received! - error freq %1.3f\n", GC.ErrorFreq / 1000.0);

  
  /*
   *---------------------------------------------------------------------------
   * Henter tid. Standser ved fejl.
   */
  if ( gettimeofday( &tv, &tz) )
    {
      perror("receiver - reading systemtime");
      Stop();
    }
                           /* tiden 'nulstilles' */
  GC.systime = (tv.tv_sec*1000000 + tv.tv_usec) / 1000;

  SC.ready   = YES;        /* globale 'ready-flag' */

  /* startsignal til de �vrige threads */
  pthread_cond_broadcast( &SC.systemsignal );
  
  /* ignorer ctrl-c                    */
  signal(SIGINT, SIG_IGN);

  /*
   *---------------------------------------------------------------------------
   * Funktionen sætter sig nu til at vente på at der dukker noget op...
   */

  while(1)
    {
      if (read(GC.CtrlSocket[ThisStation].sock,
	       (char *)&buffer, sizeof(buffer)) < 0)
	perror("Receiving datagram");

      if (( buffer.type == (unsigned char)ctrl_c ) ||  	/* så stopper vi! */
	  ( buffer.type == (unsigned char)stop ))

	{
	  if (buffer.type == (unsigned char)ctrl_c )
	    {
	      usleep(300000);
	      sprintf(msg,  "<<ctrl-C signal from 'network' received.>>\n");
	      LOGINFO( msg, strlen(msg) );
	      
	      if (GC.xterm)
		printf("%s", msg);
	    }
	  
	  /* lukker stationen */
	  close_station(CONTROL);
	}
      
      /* ny øvre grænse for sekvensnumre modtaget */
      if ( buffer.type == (unsigned char)credit )
	{
	  /* Opdaterer øvre grænse for sekvensnummer */
	  LOCK( &FC[buffer.station].lock, control );
	  FC[buffer.station].upper_limit = buffer.limit;
	  UNLOCK( &FC[buffer.station].lock, control );

	  pthread_cond_signal( &FC[buffer.station].signal );
	}

    }
#if !defined (__sun)
	  return NULL;
#endif
}
