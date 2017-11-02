/* 	$Id: errortimer.c,v 1.10 1999/09/04 12:08:57 dim Exp $	
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Denne funktion kører som thread. Den sørger for at flytte
 * forsinket data delay-bufferen til den normale inddata-kø. Dataene gø ikke
 * direkte, men via en 'statisk' fifokø - TempTransfer. Det er nødvendigt, da
 * der er mulighed for deadlocks, når 'errortimer' læser fejldatakøen, og
 * samtidig skal allokerer data dynamisk.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: errortimer.c,v 1.10 1999/09/04 12:08:57 dim Exp $";
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "subnet.macro"
#include "subnet.h"
#include "subnet.type"
#include "subnet_internal.h"

extern GlobalControlStruct GC;           /* samlet kontrolstruktur */
extern StateControlStruct  SC;           /* Systemets tilstand */

extern log_type            LogStyle;     /* hvordan skal systemet føre log? */
extern pthread_key_t       id;           /* til identifikation af threaden */

void *errortimer(void *arg)
{
  char msg[BUFCTRLSIZE + BUFFERSIZE + 100];
  BufStr tempbuffer;
  PriQueueEntry TempTransfer[ERRORTRANSFMAX];       /* til statisk fifokø */
  register int TmpTraP, TmpTraMax;        /* 'pointer', og 'max' til fifo */
  int msglen, cnt = 0;

  /*
   * threaden 'husker' sit eget id og navn.
   */
  pthread_setspecific( id, malloc( sizeof( void * )) );
  *((intptr_t *)pthread_getspecific( id )) = (intptr_t)arg;

  GC.ThArr[(intptr_t)arg]->name = strcpy( (char *) malloc (sizeof(char)* 22), 
 				     "(SYSTEM) errortimer"); 

  /* venter p� et GO fra hovedprogrammet */
  LOCK( &SC.systemlock, errortimer );
  pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, errortimer );

  while(1)
  {
      /* undersøger om stationen er klar eller ved at lukke ned (ved fejl) */
    SYSTEMOK( errortimer );

    LOCK( &GC.DelayData.errorlock, errortimer );

      /* Hvis der ikke er noget data i denne kø, så vent på et signal om at */
      /* der er kommet data fejl-køen.                                      */
    if (EmptyPQ( GC.DelayData.ErrorDataQ ))
        pthread_cond_wait( &GC.DelayData.errorsignal, &GC.DelayData.errorlock );

      /*
       * Alle de pakker hvor tiden er udløbet, skal fjernes fra køen, inden
       * låsen slippes, og der kan ikke allokeres mere hukommelse dynamisk
       * inden låsen er sluppet, pga. faren for deadlocks.
       */
    if (GC.DelayData.errorcount == FindMinKeyPQ( GC.DelayData.ErrorDataQ ))
    {
        TmpTraP = TmpTraMax = 0;

	  /* elementer fjernes fra fejldatakøen */
        while (GC.DelayData.errorcount
                == FindMinKeyPQ( GC.DelayData.ErrorDataQ ))
        {
            TempTransfer[TmpTraP] = DeleteMinPQ( GC.DelayData.ErrorDataQ );
            TmpTraP++;
            TmpTraMax++;
        }
        /*
         * Alt med udløbet tidsfrist er nu fjernet fra køen, og der kan
         * låses op, og overførslen tl den normale inddatakø kan begynde
         */
        UNLOCK( &GC.DelayData.errorlock, errortimer );
	  
        for(TmpTraP = 0; TmpTraP < TmpTraMax; TmpTraP++)
        { 
            /* kopierer BufferStruct-indholdet af DelayDataQ-elementet     */
            tempbuffer = (BufStr)memcopy((char *)calloc(sizeof(BufStrStruct),
                         sizeof(char)),
                         ( (DelayDataElement)ValueOfPQE(
                         TempTransfer[TmpTraP] ))->data,
                         sizeof(BufStrStruct) );
 
            /* Også her er der mulighed for gentagelser                    */
            if (((DelayDataElement)ValueOfPQE(
                    TempTransfer[TmpTraP] ))->multiple_ok 
                && (( rand() % RATIO ) < MULTIPLERATIO ))
            {
                /* dubleres */
                re_delay_frame(TempTransfer[TmpTraP]);

                sprintf(msg, "multiplying delayed buffer(#%d,%d): ",
                        tempbuffer->seq, tempbuffer->station);
            }
            else
            {
                /* Tiden er udløbet,  DelayData-elementet kan slettes.     */
                free( (DelayDataElement)ValueOfPQE( TempTransfer[TmpTraP] ) );
                sprintf( msg, "receiving delayed buffer(#%d,%d): ",
                         tempbuffer->seq, tempbuffer->station);
            }

            /* Der dannes en log-meddelse */
            msglen = strlen(msg);
            memcopy((char *)msg + msglen, tempbuffer->data, tempbuffer->size);
            msg[msglen + tempbuffer->size] = 10; /* linieskift */

            /* Prioritetskø-elementerne fjernes. (elementer fra ErrorDataQ) */
            DeletePQE( TempTransfer[TmpTraP] );

            LOGINFO( msg, (msglen + tempbuffer->size +1) );

            /* BufStr-elementet overf�res til den normale datak�.           */
            transfer_frame( tempbuffer );

            /* For at sikre at der ikke modtages for meget data ad gangen.  */
            if (++cnt >= YIELDLIMIT)
            {
                cnt = 0;
                sched_yield(); /* processkift */
            }

        } /* end-for */

        cnt = 0;                 /* klar til ny omgang */
        LOCK( &GC.DelayData.errorlock, errortimer );
        GC.DelayData.errorcount++;
        UNLOCK( &GC.DelayData.errorlock, errortimer );
        continue;

    }  /* end-if, slut p� overf�rsel */

    GC.DelayData.errorcount++;
    UNLOCK( &GC.DelayData.errorlock, errortimer );

    sched_yield(); /* processkift */
  }
#if !defined (__sun)
   return NULL;
#endif
}
