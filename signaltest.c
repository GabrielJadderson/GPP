/* 	$Id: signaltest.c,v 1.6 1999/09/06 16:17:27 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: Viser brugen af Signal, Wait og ClearEvent. 
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: signaltest.c,v 1.6 1999/09/06 16:17:27 dim Exp $";
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <subnet.h>
#include <subnetsupport.h>

/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

/* Events */
#define MSG            0x04
#define aux1           0x08
#define aux2           0x10
#define STOP           0x20

/* Globale variable */

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */
log_type LogStyle = nolog; /* Hvilken slags log skal systemet føre            */
                               
LogBuf mylog;                /* logbufferen                                     */

/*
 * Funktion der sender signaler
 */
void Sender()
{
  char *msg;
  char logmsg[100];
  const char startmsg[] = "Start Sender\n";
  intptr_t i;

  Log(mylog, startmsg, strlen(startmsg));

  /* sender 10 'MSG', 'aux1' og 'aux2' signaler */
  for (i = 1; i <= 10; i++)
    {
      msg = (char *) malloc (sizeof(char) * 32);
      sprintf( msg, "besked %d.", (int)i);
      Signal( MSG, (void *)msg);    /* signal + en pointer til en streng ...*/ 
      Signal( aux1, (void *)i );    /* signal + en integer ...              */
      Signal( aux2, NULL );         /* signal ...                           */

      sprintf( logmsg,
	       "(%s) i = %d, besked = \"%s\"\n",
	       GetProcessName(), (int)i, msg);
      Log( mylog, logmsg, strlen(logmsg));
    }

  /* sender STOP event */
  sprintf( logmsg, "(%s) Sender 'STOP'-event\n", GetProcessName());
  Signal( STOP , NULL);
  Log( mylog, logmsg, strlen(logmsg));
}

/*
 * Funktion der venter på signaler
 */
void Receiver()
{
  char msg[100];
  const char startmsg[] = "Start Receiver\n";
  int res, FLAG = 1;
  event_t ev;

  Log(mylog, startmsg, strlen(startmsg));

  while(FLAG)
    {
      /* venter på tre forskellige events: 'MSG', 'STOP' og 'aux1' */
      res = Wait(&ev, MSG | STOP | aux1);
      
      if (res == MSG)
	sprintf(msg,
		"Har modtaget signal %#6.4x: \"%s\"\n",
		(unsigned int)ev.type, (char *)ev.msg);
      else if (res == aux1)
	sprintf(msg,
		"Har modtaget signal %#6.4x: (%d)\n",
		(unsigned int)ev.type, (int)((intptr_t)ev.msg));
      else
	{
	  sprintf(msg, 
		  "Har modtaget signal %#6.4x, dvs. stop...\n",
		  (unsigned int)ev.type);
	  FLAG = 0; /* stop */
	}
      Log( mylog, msg, strlen(msg));
      printf("%s", msg);
    }

  sleep(3);
  Stop();
}

/*
 * Funktion der 'clearer' signaler
 */
void Clear()
{
  char msg[100];
  const char startmsg[] = "Start Clear\n";

  Log(mylog, startmsg, strlen(startmsg));

  while(1)
    {
      sleep(1);
      sprintf( msg, "ClearEvent (aux2) = %d\n", ClearEvent( aux2 ) );
      Log( mylog, msg, strlen(msg));
      printf("%s", msg);
    }
}

int main(int argc, char *argv[])
{
  StationName = argv[0];
  ThisStation = atoi( argv[1] );

  mylog = InitializeLB("signal");

  /* processerne aktiveres */
  ACTIVATE(1, Sender);
  ACTIVATE(1, Receiver);
  ACTIVATE(1, Clear);
 
  /* simuleringen starter */
  Start();
  exit(0);
}
