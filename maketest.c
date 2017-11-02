/* 	$Id: maketest.c,v 1.5 1999/09/06 16:17:42 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: Minimalt hovedprogram. Til illustration af makefilen. 
 *
 * Steffen Schmidt.
 */
#include <subnet.h>
#include <stdlib.h>
#include <unistd.h>

char *StationName;         /* Globalvariabel til at overføre programnavn       */
int ThisStation;           /* Globalvariabel der identificerer denne station.  */
log_type LogStyle = nolog; /* Hvilken slags log skal systemet føre             */
                           /* mulige: 'nolog', 'separate' eller 'synchronized' */

/* Funktionen kompileres separat */
void funk1();

void proces()
{
  int i;

  for (i = 0; i < 10; i++)
    funk1();

  sleep(2);
}  

int main(int argc, char *argv[])
{
  StationName = argv[0];
  ThisStation = atoi(argv[1]);

  Activate(1, proces, "proces1");
  Activate(1, proces, "proces2");
  Activate(2, proces, "proces");

  Start();
  exit(0);
}
