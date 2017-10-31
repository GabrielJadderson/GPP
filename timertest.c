/* 	$Id: timertest.c,v 1.10 1999/09/06 16:17:05 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: Viser brugen af GetProcessName, GetTime, SetTimer, GetLastTid
 * og StopTimer.
 *
 * Steffen Schmidt. 
 */
/*
static char rcsid[] = "$Id: timertest.c,v 1.10 1999/09/06 16:17:05 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include <subnet.h>
#include <subnetsupport.h>
#include <unistd.h>

char *StationName;         /* Global variabel til at overføre programnavn      */
int ThisStation;           /* Global variabel der identificerer denne station. */
log_type LogStyle = nolog; /* Hvilken slags log skal systemet føre             */
                           /* mulige: 'nolog', 'separate' eller 'synchronized' */

#define MAX 10000

/* funktion der sender 10 timeout signaler */
void Timer1(void)
{
  int id, a = 1;
  char *msg;

  /* Sætter 12 timere */
  while(a <= 12)
    {
      /* besked der leveres med 'timeout' signalet */
      msg = (char *)malloc(100*sizeof(char));
      sprintf(msg, "<%s>Timeout signal %d (adr. %#x)", GetProcessName(), a, (unsigned int)((intptr_t)msg));
      
      /* sætter timeren, og giver en pointer med til beskeden */
      id = SetTimer((a * 1000), (void *)msg );

      /* Skriver hvilket nummer timeout og hvornår det blev sendt */
      printf("Process: %s: a = %d - Time %d, timer id = %d\n",
	     GetProcessName(), a++, (int)GetTime(), id);
    }
  
  /* standser de 5 første timere, og udskriver besked */
  for (a=1; a<= 5; a++)
    {
      if (StopTimer(a, (void *)&msg))
	printf("timer %d stoppet. msg: %s (msg adr: %#x)\n", a, msg, (unsigned int)((intptr_t)msg));
      free(msg);
    }

  sleep(15);
  Stop();
}

/*
 * Funktionen sætter MAX antal timere med tilfældig forsinkelse. Derefter slettes
 * alle timere, uden der er fanget nogen signaler. Der tælles hvor mange timeout
 * signaler der blev stoppet inden alle blev clearet. Alle skulle være stoppet
 * og ingen clearet.
 *
 * Derefter startes den MAX nye timere op, der ventes på timer med id 1,5 * MAX,
 * resten af timerne der er udløbet cleares og resten af de ventende timere
 * stoppes. Disse tre tal lagt sammen skal give MAX.
 */
void Timer2(void)
{
  unsigned int id, stopid;
  event_t event;
  int a = 0, cnt = 0, clear = 0, fetch = 0;
  void *msg;

  while(a < MAX)
    {
      /* s�tter timeren, og giver en pointer med til beskeden */
      id = SetTimer(4000 +(int) (4000.0*rand()/(RAND_MAX+1.0)), NULL );

      a++;
      /* Skriver hvilket nummer timeout og hvornår det blev sendt */
      if ((a % 2500) == 0)
	printf("Timer-id %d sat.\n", id);
    }

  printf("Nu fors�ges alle timere stoppet:\n");

  /* Slet aktive timere. Tæl hvor mange der blev slettet */
  for (id = 1; id <= MAX; id++){
    if ((id % 2500) == 0)
	printf("Timer-id %d checket.\n", id);

    cnt += StopTimer(id, &msg);
    free( msg );
  }
    
  printf("Resten 'cleares' vha. ClearEvent\n");
  clear = ClearEvent(timeout);

  printf("\nAntal satte timere:\t%5d\n", MAX);
  printf("Antal stoppede timere:\t%5d\n", cnt);
  printf("Antal clearet:\t\t%5d\n", clear);
  printf("\n%d + %d = %d =?= %d: STEMMER%s!\n",
	 cnt, clear, cnt + clear, MAX, (MAX == (cnt + clear) ? "" : " IKKE"));

  printf("Det sættes %d nye timere:\n", MAX);

  while(a < 2 * MAX)
    {
      /* sætter timeren, og giver en pointer med til beskeden */
      id = SetTimer(1000 + (int) (4000.0*rand()/(RAND_MAX+1.0)), NULL );

      a++;
      /* Skriver hvilket nummer timeout og hvornår det blev sendt */
      if ((a % 2500) == 0)
	printf("Timer-id %d sat.\n", id);
    }

  printf("Der hentes timeouts indtil id = %d:\n", stopid = (MAX + MAX / 2) );
  
  while(1)
    {
      Wait(&event, timeout);
      if ((++fetch % 200) == 0)
	printf(".");

      if (event.timer_id == stopid)
	break;
    }

  printf("\nDer cleares vha. ClearEvent\n");
  clear = ClearEvent(timeout);
  
  printf("Resten stoppes:\n");

  cnt = 0;

  /* Slet aktive timere. Tæl hvor mange der blev slettet */
  for (id = MAX; id <= 2 * MAX; id++)
    {
      cnt += StopTimer(id, &msg);
      free( msg );
    }

  printf("\nAntal satte timere:\t%5d\n", MAX);
  printf("Antal hentede timeouts:\t%5d\n", fetch);
  printf("Antal clearet:\t\t%5d\n", clear);
  printf("Antal stoppede timere:\t%5d\n", cnt);
  printf("\n%d + %d + %d = %d =?= %d: STEMMER%s!\n",
	 cnt, clear, fetch, cnt + clear + fetch, MAX, 
	 (MAX == (cnt + clear + fetch) ? "" : " IKKE"));
}

/* funktion der venter på timeout signaler */
void WaitTimer()
{
  event_t event;
  
  /* venter i 7 sek. inden der ventes på timeout signaler */
  if (ThisStation == 2)
    sleep(7);

  /* på timeout signaler */
  while(1)
    {
      Wait(&event, timeout);
      printf("Signal modtaget: \"%s\". Aktuel tid: %d, id %d\n",
	     (char *)event.msg, (int)GetTime(), GetLastTid());
    }
}

int main(int argc, char *argv[])
{
  StationName = argv[0];
  ThisStation = atoi(argv[1]);

  /*
   * Afgør vha. antallet af argumenter om det er test-forløb 1 eller
   * test-forløb 2 der skal køres.
   *
   * Test-forløb 1 går ud på at sætte timere og se hvordan der bliver sendt
   * beskeder med de enkelte timeout signaler.
   *
   * Test-forløb 2 er for at vise brugen af StopTimer, og at der ikke bliver
   * signaler væk i det hele taget.
   */
  if (argc == 2)
    {
      Activate(1, Timer1, "Timer1");
      Activate(1, WaitTimer, "WaitTimer");
      
      Activate(2, Timer1, "Timer1");
      Activate(2, WaitTimer, "WaitTimer");
      
      /* NB! samme funktion startet to gange! */
      Activate(3, Timer1, "Timer1(1)");
      Activate(3, Timer1, "Timer1(2)");
      Activate(3, WaitTimer, "WaitTimer");
    }
  else
    Activate(1, Timer2, "Timer2");

  Start();
  exit(0);
}
