/* 	$Id: subnet.c,v 1.16 1999/09/02 21:56:10 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: Indeholder statiske initialiseringer af global data, samt et
 * par små funktioner der ikke hører til noget bestemt sted.
 *
 * Steffen Schmidt.
 */
/*
static char vcid[]  = "$RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $";
static char rcsid[] = "$Id: subnet.c,v 1.16 1999/09/02 21:56:10 dim Exp $";
*/
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

/*
 * globale variable:
 */
extern char        *StationName;        /* pointer til argv[0] */
extern log_type     LogStyle;           /* hvordan skal systemet føre log? */
extern int          ThisStation;        /* hvilken station er dette? */

GlobalControlStruct GC   = {NULL,       /* fifo-køen */
			    0,          /* antal threads */
			    NULL,       /* ThreadControl */
			    PTHREAD_MUTEX_INITIALIZER,
			    NULL,       /* SocketControl (data) */
			    NULL,       /* SocketControl (kontrol) */
			    0,          /* fejl frekvens (i tusinddele)*/
			    0,          /* systemtid */
			    NO,         /* startet i x-term vindue */
			    0,          /* antal stationer */
			    NULL,       /* aktive stationer */
			    {NULL,      /* data kø */
			     0,         /* størrelse af kø */
			     PTHREAD_MUTEX_INITIALIZER},    /* InDataControl */
			    {NULL,      /* fejl data kø */
			     0,
			     PTHREAD_MUTEX_INITIALIZER,
			     PTHREAD_COND_INITIALIZER},     /* DelayDataControl */
			    {0,
			     0,
			     0,
			     0,
			     0,
			     0},                            /* StationStat   */
			    NULL        /* logbufferen */
};

EventControlStruct  EC;     /* events */

FlowControl         FC;     /* flow control */   

StateControlStruct  SC   = {NO,         /* system ready */
			    0,          /* 'shell' threads alive */
			    PTHREAD_MUTEX_INITIALIZER,
			    PTHREAD_COND_INITIALIZER};

TimeoutControlStruct TC  = {NULL,
			    NULL,
			    NULL,
			    1,           /* f�rste timeout id */
			    PTHREAD_MUTEX_INITIALIZER,
			    PTHREAD_COND_INITIALIZER};

unsigned int         _last_timeout_id = 0; /* id for sidst hentet timeoutsignal */
pthread_mutex_t      _last_tid_lock   =	PTHREAD_MUTEX_INITIALIZER;

pthread_key_t        id;                 /* thread specifik data */

/*
 * Egen implementation  af 'memcpy'. Defineret for at undgå kritiske regioner.
 * Da der ikke er brugt static data kan de enkelte threads benytte denne funktion
 * uden en lås, da der ikke vil opstå interferens fra andre threads.
 */
inline void *memcopy(void *dest, const void *src, size_t n)
{
  register int i;

  for (i = 0; i < n; i++)
    *((char *)dest + i)  = *((char *)src + i);

  return dest;
}

/*
 * Denne funktion lukker stationen. Den kan kaldes af receiver og control,
 * de to tråde der kan modtage stop-signaler på hhv. datasocket og kontrol-
 * socket.
 */
void close_station(int caller)
{
    /*
  BufStrStruct  data_buffer = {stop, ThisStation, 0, 0};
  BufCtrlStruct ctrl_buffer = {stop, ThisStation, 0};*/
  BufStrStruct *data_buffer = (BufStrStruct *)malloc(sizeof(BufStrStruct));
  BufCtrlStruct *ctrl_buffer = (BufCtrlStruct *)malloc(sizeof(BufCtrlStruct));
  SocketControl TempSocket;
  char *buffer;
  int size;
  
  data_buffer->type = stop;
  data_buffer->station = ThisStation;
  data_buffer->seq = 0;
  data_buffer->size = 0;
  ctrl_buffer->type = stop;
  ctrl_buffer->station = ThisStation;
  ctrl_buffer->limit = 0;
  /* unders�ger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( close_station );

  /*
   * For at stoppe afsendelser af data, samt sikre denne funktion kun udføres
   * en gang.
   */
  LOCK( &SC.systemlock, receiver );
  SC.ready = STOPPING;
  UNLOCK( &SC.systemlock, receiver );

#ifdef  DEBUG_CONTSW 
  sprintf( msg, ">> %d: Number of context switches per thread.\n",
	   context_switch);
  if (GC.xterm)
    printf("%s", msg);
  
  LOGINFO( msg, strlen(msg) );
#endif

  usleep(300000);

  FlushAllLog();

  /* Hvem har kaldt close_station? Der skal sendes stop-signal til den anden
     thread, pga. disse threads står i en blokerende read på hver deres socket,
     og den nemmeste måde at komme i kontakt med dem er via data på denne
     socket. */
  if (caller == CONTROL) 
    {
      TempSocket = &GC.SocketArr[ThisStation];
      buffer     = (char *)data_buffer;
      size       = sizeof(BufStrStruct);
    }
  else
    {
      TempSocket = &GC.CtrlSocket[ThisStation];
      buffer     = (char *)ctrl_buffer;
      size       = sizeof(BufCtrlStruct);
    }

  /* låser socketen før der sendes */
  LOCK( &GC.OwnSocketLock, close_station );
  
send_stop_label:
  if (sendto( TempSocket->sock, buffer, size, 0,
	      (struct sockaddr *)&TempSocket->name,
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

  /* anden arg. til shutdown (2) betyder: "lukket for både read og write" */
  shutdown(GC.SocketArr[ThisStation].sock, 2);          /* lukker socket  */
  shutdown(GC.CtrlSocket[ThisStation].sock, 2);         /*      do.       */
  unlink(GC.SocketArr[ThisStation].name.sun_path);      /* fjerner socket */
  unlink(GC.CtrlSocket[ThisStation].name.sun_path);     /*      do.       */

  UNLOCK( &GC.OwnSocketLock, close_station );  

  LOCK( &SC.systemlock, close_station );
  SC.ready = NO;
  UNLOCK( &SC.systemlock, close_station );

  /* resetter ctrl-C */
  signal(SIGINT, SIG_DFL);

  if (GC.xterm){
    printf(">> Press enter to terminate!\n");
    while ( getchar() != 10 );
  }

  free(data_buffer);
  free(ctrl_buffer);
  exit(0);
}

/*******************************************************************************
 *******************************************************************************
 *
 * Hjælpefunktioner
 *
 *******************************************************************************
 */

/*
 * Udskriver listen med ventende timeouts
 */
void printTimeoutQ()
{
  ActiveTimeoutControl currAct = TC.first;
  
  printf(":: ActiveTimerQ:\t");
  while( currAct != NULL )
    {
      printf("id = %d\t", currAct->id);
      currAct = currAct->next;
    }
  printf("\n");
}

/*
 * Finder element i den hægtede liste med timeouts
 */
inline ActiveTimeoutControl lookupTimeoutId(unsigned int id)
{
  int FOUND = FALSE;
  ActiveTimeoutControl currAct = TC.first;
  
  while( !FOUND && (currAct != NULL ))
    {
      if (currAct->id == id)      /* korrekt id fundet */
	{
	  FOUND = TRUE;
	  break;
	}
      else if (currAct->id > id)  /* id er for lavt, dvs. ikke fundet */
	break;
      
      currAct = currAct->next;
    }

  if (FOUND)
    return currAct;
  else
    return NULL;
}
/*
 * Fjerner en timeout-id fra den hægtedeliste af aktive timeouts.
 */
inline void removeTimeout(ActiveTimeoutControl act)
{
  if (act == NULL)
    return;

  /* Fjerner timeouten fra listen */
  if (act->prev == NULL)  /* det første element */
    TC.first = act->next;
  else
    act->prev->next = act->next;
  
  if (act->next == NULL)  /* det sidste element */
    TC.last = act->prev;
  else
    act->next->prev = act->prev;
  
  free( act );
}


/*
 * Udskriver indholdet af en datapakke.
 */
void PrintBuf(void *e)
{
  BufStr temp = (BufStr)((DelayDataElement)ValueOfPQE( e ))->data;
  char msg[BUFCTRLSIZE + BUFFERSIZE + 50]; 
  int msglen;

  sprintf(msg, "source %d, length %d : ", temp->station, temp->size);
  msglen = strlen(msg);
  memcopy((char *)msg, (char *)temp->data, temp->size);
  msg[msglen + temp->size] = 10;

  LOGINFO( msg, (msglen + temp->size + 1) );

  return;
}

/*
 * udskriver navn (hvis muligt)
 */
void PrintThreadName(void)
{
  /* Systemmet kører ikke endnu */
  if (SC.ready != YES)
    return;

  /* udskriv navnet */
  printf("%s", GetProcessName() );
  return;
}
