/* 	$Id: Activate.c,v 1.3 1999/08/18 12:25:11 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Sætter en funktion ind i en fifo-kø til senere start som thread,
 * hvis Station = ThisStation, dvs. gyldig aktivering.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: Activate.c,v 1.3 1999/08/18 12:25:11 dim Exp $";
*/
#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include <stdlib.h>
#include <string.h>

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern int                 ThisStation;  /* nr. på stationen (fra arg. listen) */

void Activate(int Station, void (*Process)(void), const char *name)
{
  static int INITIALIZED = NO;
  ThreadControl tcp;
  
  /* Denne proces skal ikke startes i denne station */
  if (Station != ThisStation)
    return;

  /* For at sikre, at der kun bliver initialiseret en gang */
  if (!INITIALIZED)
    {
      GC.FQ       = InitializeFQ();
      INITIALIZED = YES;
    }

  /*
   * Der skabes et element indeholdende den aktiverede proces til inds�ttelse
   * i fifo-k�en.
   */

  tcp = (ThreadControl) malloc (sizeof(ThreadControlStruct));
  tcp->process = Process;
  tcp->name    = strcpy((char *) malloc (strlen(name)+1), name);
  
  EnqueueFQ(NewFQE((void *)tcp), GC.FQ);               /* Sættes ind... */

  /* tæller antaller af aktiverede processer op (bliver til threads...) */
  GC.NumOfThreads++;

  return;
}
