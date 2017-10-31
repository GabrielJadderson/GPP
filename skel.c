/* 	$Id: skel.c,v 1.6 1999/09/06 16:17:54 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 * 
 * BESKRIVELSE: 'Skellet' til implementation af en station til 'network'
 * programmet. Som et minimum skal en netstation implementation indeholde dette.
 *
 * Steffen Schmidt.
 */

#include <subnet.h>

char *StationName;         /* Global variabel til at overføre programnavn.     */
int ThisStation;           /* Global variabel der identificerer denne station.*/
log_type LogStyle = nolog; /* Hvilken slags log skal systemet føre             */
                           /* mulige: 'nolog', 'separate' eller 'synchronized' */

int main(int argc, char *argv[])
{
  StationName = argv[0];
  ThisStation = atoi(argv[1]);
}
