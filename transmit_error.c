/* 	$Id: transmit_error.c,v 1.4 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Returnerer fejltypen for en enkelt datapakke.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: transmit_error.c,v 1.4 1999/08/18 12:25:11 dim Exp $";
*/
#include <stdlib.h>
#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"

extern GlobalControlStruct GC;  /* samlet kontrolstruktur */

error_type transmit_error(void)
{
  int res;

  /*
   * Skal der generes fejl. BEMÆRK!! Det tal 'rand' returnerer skiftes 8 bits til
   * højre. Grunden er, at de laveste bits IKKE er særlige tilfældige (i følge
   * 'man' siderne)
   */

  if (((rand() >> 8) % MAXERRORFREQ) < GC.ErrorFreq)
    {
      res = (rand() >> 8) % RATIO;

      if (res < LOOSERATIO)                   /* skal pakken smides væk */
	return loose;
	
      if (res < LOOSERATIO + DELAYRATIO)      /* skal pakken forsinkes   */
	return delay;
      else
	if (MULTIPLELIMIT > 0)
	  return multiple;                    /* en dublet (hvis dubletter er */
	else                                  /* tilladte), ellers retureres  */
	  return none;                        /* 'ingen fejl'                 */
    }
  else
    return none;                              /* ingen fejl!             */
}
