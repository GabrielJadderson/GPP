/* 	$Id: subnet.h,v 1.8 1999/09/04 12:09:33 dim Exp $	
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Denne headerfil viser hvad brugerflade til subnet indeholder. 
 * Disse er de nødvendige funktioner for at kunne køre subnet.
 * 
 * Steffen Schmidt.
 */
#ifndef _SUBNET_H_
#define _SUBNET_H_

/* macro'er: */
#define BUFFERSIZE   64           /* Størrelsen af de datapakker der overføres */
                                  /* via subnettet. (i bytes)                  */

                                  /* Pre-definerede event:                     */
#define frame_arrival 0x00000001  /* Der er data fra subnettet.                */

/* typer: */
typedef struct {                  /* event type.                               */
  long int      type;
  void         *msg;
  unsigned int  timer_id;
} event_t;

/* Hvilken slags log skal systemet udskrive (til fil) */
typedef enum { nolog,           /* Ingen log.                                  */
	       separate,        /* Station skriver til egen log "system.log.?" */
	       synchronized     /* Alle stationer skriver til "network.log"    */
} log_type;

/*
 * Prototyper: (opsætning)
 * -----------------------------------------------------------------------------
 *
 * Starter en funktion (Process) af typen "void func(void)" som proces i station
 * nr. (Stationen) i netværket. (name) er navnet på funktionen; bruges til
 * fejlrapportering.
 */
void Activate(int Station, void (*Process)(void), const char *name);

/*
 * Start og stop af simulering.
 */
void Start();
void Stop();

/*
 * Prototyper: (kommunikation)
 * -----------------------------------------------------------------------------
 *
 * "Signal" 'sætter' en begivenhed, og "Wait" venter på en eller flere begiven-
 * heder. Wait blokerer indtil en en angivet event dukker op. Er der flere events
 * sat, vil even'ten med mindst numerisk værdi returneres. "ClearEvent" fjerner
 * en event, og returnerer antallet af events der blev fjernet.
 */
void Signal(long int event, void *msg);
long int Wait(event_t *result, long int ev_bitmap);
int ClearEvent(long int event); 

/*
 * Sender og modtager via subnettet. "ToSubnet" returnerer 0 ved succes, og -1
 * ved fejl. "FromSubnet" henter en pakke af gangen fra subnettet. Funktionen
 * returnerer -1 ved fejl (f.eks. ingen data), 1 hvis der er flere datapakker i
 * bufferen og 0 ved sidste datapakke fra bufferen.
 */
int ToSubnet(int source, int dest, char *buffer, int length);
int FromSubnet(int *source, int *dest, char *buffer, int *length);
#endif
