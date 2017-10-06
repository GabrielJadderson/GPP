/* 	$Id: nettest.c,v 1.10 1999/09/06 16:16:50 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: Viser brugen af subnet-biblioteket. Skal startes som 3 stationer.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: nettest.c,v 1.10 1999/09/06 16:16:50 dim Exp $";
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <subnet.h>
#include <subnetsupport.h>
#include <fifoqueue.h>

/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

/* Events */
#define start_sending  0x04
#define data_in_queue  0x08
#define aux            0x10

/* Globale variable */

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */
log_type LogStyle;         /* Hvilken slags log skal systemet føre            */
                               
LogBuf mylog;                /* logbufferen                                     */
FifoQueue dataQ;           /* Datakø                                          */

mlock_t *queuelock;

/* Afsender proces. Kører i station 1 */
void Sender1()
{
  int a = 0;
  char buffer[100];

  sprintf( buffer, "Starttid %d\n", (int)GetTime() );
  Log( mylog, buffer, strlen(buffer) );
  
  while(a++ < 200)
    {
      /* datablokke til afsendelse... */
      sprintf(buffer, "datablok nummer: [%d], fra (%d)", a, ThisStation );

      /* sendes.. */
      ToSubnet(ThisStation, 2, buffer, strlen(buffer));
    }

  sprintf( buffer, "Stoptid %d\n", (int)GetTime() );
  Log( mylog, buffer, strlen(buffer) );

  printf("Station %d f�rdig. - (\'sleep(5)\')\n", ThisStation);

  /* en lille pause, så de øvrige stationer kan nå at blive færdig */
  sleep(5);
  Stop();
}

/* Afsender proces. Kører i station 2 */
void Sender2()
{
  char buffer[100];
  int res = 0;  
  FifoQueueEntry e;
  event_t event;

  /* venter på at blive vækket */
  while (res != start_sending )
    {
      /* der ventes på to 'events': 'start_sending' og 'aux' */
      res = Wait( &event, start_sending | aux );
      printf("(%s):event %#6.4x, besked: %s\n",
	     GetProcessName(), (unsigned int)event.type, (char *)event.msg);
    }

  sprintf( buffer, "Sender2 v�kket - starttid %d\n", (int)GetTime() );
  Log( mylog, buffer, strlen(buffer) );
  
  while(1)
    {
      Wait( &event, data_in_queue );
      
      /* Låser den kritiske region */
      Lock( queuelock );

      /* tester for fejl */
      if (EmptyFQ( dataQ ))
	{
	  /* Låser op */
	  Unlock( queuelock );

	  /* 'data_in_queue' modtaget, men køen er tom */
	  sprintf( buffer,
		   "(%s):Signal 'data_in_queue' modtaget, men k�en er tom.\n", 
		   GetProcessName());
	  printf("%s", buffer);
	  Log( mylog, buffer, strlen(buffer));
	}
      else
	{
	  /* henter element */
	  e = DequeueFQ( dataQ );

	  /* Låser op */
	  Unlock( queuelock );
	  sprintf(buffer,
		  "->:%s:<-, fra %d",
		  (char *)ValueOfFQE( e ), ThisStation );

	  /* Deallokerer buffer fra fifo-køen */
	  free( (void *)ValueOfFQE( e ) );

	  /* Deallokerer fifo-kø element */
	  DeleteFQE( e );

	  Log( mylog, "Sender: [", 9 );
	  Log( mylog, buffer, strlen(buffer) );
	  Log( mylog, "]\n" , 2);

	  /* sender bufferen til station 3 */
	  ToSubnet(ThisStation, 3, buffer, strlen(buffer));
	}
    }
}

/* Modtager proces, kører i station 2 */
void Receiver2()
{
  char buffer1[100], buffer2[100], msg3[150];
  char msg1[] = "V�gn op!";
  char msg2[] = "aux-besked";
  char *buffer3;
  int event, cnt = 0, clearcnt = 0;
  int source, dest, length;
  event_t ev;


  while(1)
    {
      /* venter på at der ankommer data */
      event = Wait(&ev, frame_arrival);

      /* fjerner ekstra signaler */
      clearcnt += ClearEvent(frame_arrival);

      /* sålænge der er data i bufferen, så læses der */
      while(-1 != FromSubnet(&source, &dest, buffer1, &length))
	{
	  /* tæller antal pakker */
	  cnt++;

	  if ((cnt % 50) == 0)
	    {
	      sprintf(msg3,
		      "(%s):Antal frames modtaget %d, clearet signaler %d\n",
		      GetProcessName(), cnt, clearcnt );
	      printf("%s", msg3);
	      Log( mylog, msg3, strlen(msg3));
	    }

	  /* nul-terminering pga. sprintf */ 
	  buffer1[length] = 0;

	  sprintf( buffer2, "Har modtaget: [%s]\n", buffer1);
	  Log(mylog, buffer2, strlen(buffer2));

	  /* kopiering og nul-terminering af indkommen data */
	  buffer3 = (char *) malloc (sizeof(char) * (length + 1));
	  memcpy(buffer3, buffer1, length);
	  buffer3[length] = 0;
	  
	  /* Griber den kritiske region */
	  Lock( queuelock );
	  EnqueueFQ( NewFQE( (void *)buffer3 ), dataQ );
	  Unlock( queuelock );
	  
	  /* Sender signal til Sender2-processen */
	  Signal( data_in_queue, NULL );

	  /* Når der er modtaget 50 pakker vækkes stationens sender.. */
	  if (cnt == 50)
	    Signal( start_sending, (void *)msg1 );
	  
	  /* sender et andet signal... */
	  if (cnt < 4)
	    Signal( aux, (void *)msg2 );
	}
    }
}

/* Modtager proces, kører i station 3 */
void Receiver3()
{
  char buffer1[100];
  char buffer2[100];
  char msg[100];
  int event, cnt = 0;
  int source, dest, length;
  event_t ev;

  while(1)
    {
      /* venter på at der ankommer data */
      event = Wait(&ev, frame_arrival);

      /* læs en frame */
      if (-1 == FromSubnet(&source, &dest, buffer1, &length))
	{
	  sprintf(msg, 
		  "Fejl ved modtagelse af dataframe: Frame %d var ikke til stede.\n",
		  cnt);
	  printf("%s", msg);
	  Log( mylog, msg, strlen(msg));
	}
      else
	{
	  /* tæller antallet af hentede frames op*/
	  cnt++;

	  /* nul-terminering pga. sprintf */ 
	  buffer1[length] = 0;

	  sprintf( buffer2, "Har modtaget: [%s]\n", buffer1);
	  Log(mylog, buffer2, strlen(buffer2));
	}
    }
}
  
int main(int argc, char *argv[])
{
  StationName = argv[0];
  ThisStation = atoi( argv[1] );

  if (argc == 3)
    printf("Station %d: arg2 = %s\n", ThisStation, argv[2]);

  mylog = InitializeLB("mytest");

  if (ThisStation == 1)
    /* der ønskes ingen separate systemlog for denne station */
    LogStyle = nolog;
  else
    LogStyle = separate;
  
  if (ThisStation == 2)
    dataQ = InitializeFQ();

  queuelock = (mlock_t *)malloc(sizeof(mlock_t));
  Init_lock( queuelock );

  /* processerne aktiveres */
  ACTIVATE(1, Sender1);
  ACTIVATE(2, Sender2);
  ACTIVATE(2, Receiver2);
  ACTIVATE(3, Receiver3);
 
  /* simuleringen starter */
  Start();
  exit(0);
}
