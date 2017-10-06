/* 	$Id: subnetsupport.c,v 1.16 1999/09/04 12:19:06 dim Exp $
 *   
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Funktioner designet til subnettet, men som ikke er nødvendige,
 * bare meget rare at have. Det drejer sig om støtte til oprettelse af kritiske
 * regioner, logning af data, udskrift af procesnavn og 'timer' funktioner.
 *
 * Steffen Schmidt.
 */
/*
static char rcsid[] = "$Id: subnetsupport.c,v 1.16 1999/09/04 12:19:06 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "subnet.h"
#include "subnet.macro"
#include "subnet.type"

#define _SUBNETSUPPORT_INC_
#include "subnet_internal.h"
#undef  _SUBNETSUPPORT_INC_

#include "subnetsupport.h"

extern StateControlStruct   SC; 
extern GlobalControlStruct  GC;
extern EventControlStruct   EC;
extern TimeoutControlStruct TC;              /* kontrol af timeouts */
extern int                  ThisStation;
extern log_type             LogStyle;        /* hvordan skal systemet føre log*/
extern pthread_key_t        id;              /* thread specifikt data */
extern unsigned int         _last_timeout_id;/* sidst hentede timeout     */
extern pthread_mutex_t      _last_tid_lock;

BufQControlStruct           BC = { NULL,     /* Log buffer queue */     
				   PTHREAD_MUTEX_INITIALIZER};

/*
 * Makro der afgør om et tegn 'c' kan udskrives. Resultatet er <> 0 hvis c kan
 * udskrives. (0x60 & c) udmasker 6. og 7. bit; disse er i alle ikke-kontrol-
 * tegn (se ascii tegntabellen), (0x7f ^ c) - xor - fjerne 127 (del) fra denne
 * mængde.
 */
#define isprint(c) ((0x60 & c) && (0x7f ^ c))

const char hex[] = {"0123456789abcdef"};      /* bruges af clean_ld */


/*
 * En lokal funktion der renser logdata for kontrolkarakterer (0 < 'x' < 32
 * og 126 < 'x' < 160 ). Returværdien er længden af outdata.
 */
inline int clean_ld(const char *indata, const int length, unsigned char **outdata)
{
  int i, out_i = 0;
  /* allokere den 4 dobbelte mængde af inddate - det er den maksimale længde
     af uddata, dvs hvis der kun var tale om kontrolkaraktere */
  *outdata = (unsigned char *) malloc (sizeof(char) * 4 * length);

  /* Gennemløber inddata tegn for tegn. Sidste tegn behandles separat */
  for (i = 0; i < length-1; i++)
    {
      if (isprint(indata[i]))
	/* normalt tegn - kopier dette */
	(*outdata)[out_i++] = indata[i];
      else
	{
	  /* kontroltegn - omskriv dette  */
	  (*outdata)[out_i++] = 171;
	  (*outdata)[out_i++] = hex[(indata[i] & 0xf0) >> 4];
	  (*outdata)[out_i++] = hex[indata[i] & 0xf];
	  (*outdata)[out_i++] = 187;
	}	  
    }
  
  /* hvis sidste tegn er et "newline", så kopieres dette. Ellers som ovenfor */
  if ( indata[length-1] == '\n'  || isprint(indata[length-1]))
    (*outdata)[out_i++] = indata[length-1];
  else
    {
      (*outdata)[out_i++] = 171;
      (*outdata)[out_i++] = hex[(indata[length-1] & 0xf0) >> 4];
      (*outdata)[out_i++] = hex[indata[length-1] & 0xf];
      (*outdata)[out_i++] = 187;
    }

  return out_i;                          /* længden af outdata */
}

/*
 * initialiserer mutex
 */
int Init_lock(mlock_t *lock)
{
  int res;
  char errormsg[100];

  if ((res = pthread_mutex_init( &lock->mutex, NULL )) != 0)
    {
      sprintf(errormsg, "Process: (%s) - initalizing lock\n", GetProcessName());
      printf("%s", errormsg);
      LOGINFO( errormsg, strlen(errormsg) );
    }
  return res;
}

/*
 * fjerner en l�s
 */
int Destroy_lock(mlock_t *lock)
{
  int res;
  char errormsg[100];

  if ((res = pthread_mutex_destroy( &lock->mutex )) != 0)
    {
      sprintf(errormsg, "Process: (%s) - destroying lock\n", GetProcessName());
      printf("%s", errormsg);
      LOGINFO( errormsg, strlen(errormsg) );
    }
  return res;
}

/*
 * l�ser mutex
 */
int Lock(mlock_t *lock)
{
  int res;
  char errormsg[100];

  if ((res = pthread_mutex_lock( &lock->mutex )) != 0)
    {
      sprintf(errormsg, "Process: (%s) - locking error\n", GetProcessName());
      printf("%s", errormsg);
      LOGINFO( errormsg, strlen(errormsg) );
    }
  return res;
}

/*
 * pr�ver at l�se mutex
 */
int Trylock(mlock_t *lock)
{
  return  pthread_mutex_trylock( &lock->mutex );
}

/*
 * l�ser mutex op.
 */
int Unlock(mlock_t *lock)
{
  int res;
  char errormsg[100];

  if ((res = pthread_mutex_unlock( &lock->mutex )) != 0)
    {
      sprintf(errormsg, "Process: (%s) - unlocking error\n", GetProcessName());
      printf("%s", errormsg);
      LOGINFO( errormsg, strlen(errormsg) );
    }
  return res;
}

/*
 * Returnerer tid i millisekunder, siden stationen blev startet.
 */
long GetTime(void)
{
  struct timeval tv;
  struct timezone tz;
  const char errormsg[] = "GetTime - reading systemtime.\n";

  /* unders�ger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( GetTime );

  /* henter tid. Standser ved fejl. */
  if ( gettimeofday( &tv, &tz ) )
    {
      LOGINFO( errormsg, strlen(errormsg) );
      perror( errormsg );
      Stop();
    }
  return ((tv.tv_sec*1000000 + tv.tv_usec) / 1000
	  - GC.systime);
}

/*
 * S�tter en timer med 'delay' antal millisekunder. '*msg' gives vidre til
 * Signal, og leveres til Wait, n�r 'timeout' signalet skal fanges. Returnerer
 * et entydigt id p� den timer der blev sat. Dette id kan bruges til at 
 * stoppe timeren igen.
 */
unsigned int SetTimer(int delay, void *msg)
{
  PriQueueEntry pqe;
  ActiveTimeoutControl act;
  TimeoutElement te;
  struct timeval tv;
  struct timezone tz;
  int t, nextId;
  const char errormsg[] = "SetTimer - reading systemtime.\n";

  /* unders�ger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( SetTimer );

  /* henter tid. Standser ved fejl. */
  if ( gettimeofday( &tv, &tz ) )
    {
      LOGINFO( errormsg, strlen(errormsg) );
      perror( errormsg );
      Stop();
    }

  /* s�tter systemtiden for timeout */
  t = ((tv.tv_sec*1000000 + tv.tv_usec) / 1000 - GC.systime) + delay;
  
  /* Element til inds�ttelse i timeout-prioritetsk�en */
  te = (TimeoutElement) malloc (sizeof(TimeoutElementStruct));

  /* henter n�ste timer id */
  LOCK( &TC.lock, SetTimer );
  nextId = TC.nextId++;
  UNLOCK( &TC.lock, SetTimer );
  
  /* Giver EventControl besked om at den f�rste timer er startet (pga. speciel
     opf�rsel i Wait for event.type == 0x0002) */
  if (nextId == 1)
    {
      LOCK( &EC.lock, SetTimer );
      EC.timer_active = YES;
      UNLOCK( &EC.lock, SetTimer );
    }

  te->id      = nextId;
  te->cleared = FALSE;
  te->msg     = msg;

  /* selve pri-Q elementet */
  pqe = NewPQE( t, te );

  /* element til den h�gtede liste af aktive timeouts */
  act = (ActiveTimeoutControl) malloc (sizeof(ActiveTimeoutControlStruct));
  act->id   = te->id;
  act->pqe  = pqe;
  act->prev = NULL;
  act->next = NULL;

  /* elementerne inds�ttes i timeoutQ og listen over aktive timeouts */ 
  LOCK( &TC.lock, SetTimer ); 

  /* i prioritets k�en */
  InsertPQ( pqe, TC.timeoutQ );

  /* i den h�gtede liste */
  if (TC.last == NULL)
    {                       /* hvis listen er tom */
      TC.first = act;
      TC.last  = act;
    }
  else
    {                       /* ellers tilf�j til enden af listen */
      TC.last->next = act;
      act->prev     = TC.last;
      TC.last       = act;
    }

  UNLOCK( &TC.lock, SetTimer );
  pthread_cond_signal( &TC.signal );

  return te->id;
}

/*
 * Stopper timer. Returnerer 1 hvis timeren blev fjernet og 0 hvis opslag
 * p� det givne id ikke gav noget resultat. Den eventuelle besked leveres
 * via *msg.
 */
int StopTimer(unsigned int id, void **msg)
{
  int FOUND = FALSE;
  ActiveTimeoutControl currAct;
  register FifoQueueEntry currE;

  SYSTEMOK( StopTimer );
  LOCK( &TC.lock, StopTimer );  
  
  /* sl� op p� id */
  currAct = lookupTimeoutId(id);
  *msg     = NULL;

  if (currAct != NULL)
    {
      /* Markerer timeouten som 'cleared', dvs. den skal ikke sendes n�r
	 tiden er udl�bet */
      ((TimeoutElement)ValueOfPQE(currAct->pqe))->cleared = TRUE;
      
      /* kopierer beskeden til msg */
      *msg = ((TimeoutElement)ValueOfPQE(currAct->pqe))->msg;

      /* fjerner timeout fra listen af aktive timeouts */
      removeTimeout(currAct);

      /* det blev fjernet en timer fra k�en */
      FOUND = TRUE;
    }
  UNLOCK( &TC.lock, StopTimer );

  /*
  ***************************************************************************
  *
  * De n�ste fyrre linier kode er hvad der kan betegnes som voldt�gt p� 
  * opake datatyper, i dette tilf�lde en fifok�. Det er n�sten s� skummelt
  * som det kan blive, men det er den nemmeste n�de at f� et tilf�ldigt
  * element pillet ud af fifo-k�en p�, n�r man ikke kender det enkelte k�-
  * element, men kun ved hvad indholdet af elementet skal v�re. Med andre ord:
  * Et hack! Men, som min k�re ven "eljay" engang har sagt: "Man skal engang
  * i mellem have lov til at benytte at man selv har skruet lortet sammen!!!"
  * (Frit efter hukommelsen, DM18 maj 1996, under kodegenereringsdelen).
  *
  ***************************************************************************
  */

  /* M�ske st�r signalet i event-k�en, kig efter det der. */
  if (!FOUND)
    {
      /* L�s event strukturen */
      LOCK( &EC.lock, StopTimer );
  
      /* Start ved f�rste element i k�en */
      currE = EC.msgQ[1]->first;   /* timeout = 0x02 = 2^1, derfor index=1 */

      /* leder gennem listen */
      while( !FOUND && (currE != NULL ))
	{
	  if (((TimeoutElement)currE->val)->id == id) /* korrekt id fundet */
	    {
	      FOUND = TRUE;
	      *msg = ((TimeoutElement)currE->val)->msg;
	      break;
	    } 
	  currE = currE->next;
	}

      if (FOUND)
	{
	  /* Fjerner timeouten fra listen */
	  if (currE->prev == NULL)  /* det f�rste element */
	    EC.msgQ[1]->first = currE->next;
	  else
	    currE->prev->next = currE->next;
	  
	  if (currE->next == NULL)  /* det sidste element */
	    EC.msgQ[1]->last = currE->prev;
	  else
	    currE->next->prev = currE->prev;
  
	  /* elementet deallokeres */
	  free( currE );

	  /* Hvis listen er tom: fjern event fra bitm�nster */
	  if ( EC.msgQ[1]->first == NULL )
	    EC.events &= ~timeout;
	}

      UNLOCK( &EC.lock, StopTimer );
    }

  return FOUND;
}

/*
 * Returnerer timer-id p� timer for den den sidst hentede timeout event
 * (hentet via 'Wait').
 */
unsigned int GetLastTid()
{
  unsigned int res;

  /* unders�ger om stationen er klar eller ved at lukke ned (evt. ved fejl) */
  SYSTEMOK( SetTimer );

  LOCK( &_last_tid_lock, GetLastTid );
  res = _last_timeout_id;
  UNLOCK( &_last_tid_lock, GetLastTid );

  return res;
}

/*
 * Allokerer plads, og initialiserer en ny log-buffer.
 */
LogBuf NewLogBuffer(const char *name)
{
  LogBuf lgp = (LogBuf) malloc (sizeof(LogBufStruct));
  const char errormsg1[] =
    "NewLogBuffer - error, initialize before calling \'Start\'\n";
  const char errormsg2[] = "NewLogBuffer - initializing LogBuf-mutex.\n";

  /*
   * Check om stationen er sat igang. Hvis det er tilf�ldet, s� m� der ikke 
   * initialiseres flere LogBuffere.
   */
  LOCK( &SC.systemlock, NewLogBuffer );
  if ( SC.ready == YES )   /* systemet er sat igang!! */
    {
      UNLOCK( &SC.systemlock, NewLogBuffer );
      LOGINFO( errormsg1, strlen(errormsg1));
      printf("%s", errormsg1 ); 
      Stop();
    }
  UNLOCK( &SC.systemlock, NewLogBuffer );

  lgp->name   = strcpy( (char *) malloc (sizeof(char) * strlen(name)), name );
  lgp->pos    = 0;
  lgp->num    = 1;
  lgp->logbuf = (char *) malloc (sizeof(char) * CHUNKSIZE);

  if ( pthread_mutex_init( &lgp->lock, NULL ) )
    {
      LOGINFO( errormsg2, strlen(errormsg2));
      perror( errormsg2 );
      Stop();
    }  
  return lgp;
}

/*
 * Initialiserer, og inds�tter en logbuffer i en fifok�, s�ledes bufferen
 * bliver udskrevet, n�r FlushAllLog kaldes.
 */
LogBuf InitializeLB(const char *name)
{
  static int INITIALIZED = NO;
  LogBuf lgp;
  const char errormsg[] = 
    "InitializeLB - error, initialize before calling \'Start\'\n";

  /*
   * Check om stationen er sat igang. Hvis det er tilf�ldet, s� m� der ikke 
   * initialiseres flere LogBuffere.
   */
  LOCK( &SC.systemlock, InitializeLB );
  if ( SC.ready == YES )   /* systemet er sat igang!! */
    {
      UNLOCK( &SC.systemlock, InitializeLB );

      LOGINFO( errormsg, strlen(errormsg));
      printf("%s", errormsg);
      Stop();
    }
  UNLOCK( &SC.systemlock, InitializeLB );

  LOCK( &BC.lock, InitializeLB );

  /* initaliserer fifo-k�en f�rste gang funktionen kaldes */
  if (!INITIALIZED)
    {
      BC.BufQ = InitializeFQ();
      INITIALIZED = YES;
    }
  lgp = NewLogBuffer(name);
  
  EnqueueFQ( NewFQE( (void *)lgp ), BC.BufQ );
  UNLOCK( &BC.lock, InitializeLB );   
  return lgp;
}

/*
 * Logger data i specificeret buffer. Det er v�rd at bem�rke, at der skrives til
 * hukommelsen, og ikke til en fil.
 */
void Log(LogBuf b, const char *data, int length)
{ 
  const char errormsg[] = "Log - error, non-existing buffer.\n";
  unsigned char *logdata;
  int  len_logdata;

  /*
   * For at kunne standse program udf�rslen. SYSTEMOK kan ikke bruges, fordi
   * Log skal k�re videre under b�de tilstand YES og STOPPING
   */
  LOCK( &SC.systemlock, Log );

  if (SC.ready == NO)
    pthread_cond_wait( &SC.systemsignal, &SC.systemlock );

  UNLOCK( &SC.systemlock, Log );

  if (b == NULL)
    {
      LOGINFO( errormsg, strlen(errormsg));
      printf("%s", errormsg);
      Stop();
    }

  /* renser logdata for kontroltegn */
  len_logdata = clean_ld(data, length, &logdata);

  LOCK( &b->lock , Log ); 

  /* allokerer mere plads, n�r der ikke er plads til al det nye data */
  if ( b->pos + len_logdata >= b->num * CHUNKSIZE )

    /* der realokeres mere plads */
    b->logbuf = (char *) realloc (b->logbuf, CHUNKSIZE *
				  sizeof(char) * (++b->num));
  
  /* data kopieres ind. b->logbuf er adressen hvor der startes fra, og b->pos
     er off-set i forhold til dette. */
  memcopy( (b->logbuf + b->pos), logdata, len_logdata );

  b->pos += len_logdata;         /* positionen opdateres. */
  free(logdata);

  UNLOCK( &b->lock , Log );
  return;
}

/*
 * Logger al data synkront (via network)
 */
void SyncLog(const char *loginfo, int length)
{
  BufLogStruct buffer;
  unsigned char *logdata;
  int  len_logdata;

  /*
   * For at kunne standse program udf�rslen. SYSTEMOK kan ikke bruges, fordi
   * SyncLog skal k�re videre under b�de tilstand YES og STOPPING
   */
  LOCK( &SC.systemlock, SyncLog );
  if (SC.ready == NO)
    pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, SyncLog );

  /* renser logdata for kontroltegn */
  len_logdata = clean_ld(loginfo, length, &logdata);

  buffer.type    = data;
  buffer.station = ThisStation;

  /* s�t str. til 'len_logdata' eller 'SYNCLOGSIZE', 
     alt efter hvad der er mindst */ 
  buffer.size    = ( len_logdata < SYNCLOGSIZE ? len_logdata : SYNCLOGSIZE );

  /* kopiering af data til bufferen */
  memcopy( (char *)buffer.data, logdata, buffer.size );
  free(logdata);

  /* l�s socket for afsendelse */
  LOCK( &GC.OwnSocketLock, SyncLog );

  /*
   * Der springes til dette label i tilf�lde at, at der er for meget data i k�
   * p� stationens socket.
   */
sync_log_label:

  if (sendto( GC.SocketArr[0].sock, (char *)&buffer,
	      BUFLOGCTRLSIZE + buffer.size, 0, 
	      (struct sockaddr *)&GC.SocketArr[0].name, 
	      sizeof(struct sockaddr_un)) < 0 )
    {
      /* Det g�r for hurtigt, til at maskinen kan f�lge med */
      if (errno == ENOBUFS)
	{
	  usleep(10000); /* 1/100 sek. */
	  goto sync_log_label;
	}
      
      perror("sending log-message");
    }
  UNLOCK( &GC.OwnSocketLock, SyncLog );
  return;
}

/*
 * Udskriver en buffer til fil
 */
void PrintLog(LogBuf b)
{
  time_t systime;
  char *name;
  FILE *fp;

  /*
   * For at kunne standse program udf�rslen. SYSTEMOK kan ikke bruges, fordi
   * PrintLog skal k�re videre under b�de tilstand YES og STOPPING
   */
  LOCK( &SC.systemlock, PrintLog );
  if (SC.ready == NO)
    pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, PrintLog );

  LOCK( &b->lock , PrintLog );
  
  if (b->pos > 0)
    {
      /* 
       * For at sikre der ikke skrives for meget data ud, s� sikres det, at
       * dataen i LogBufferen er 0-termineret. Positionen '(b->logbuf + b->pos)'
       * ER allokeret (i Log), og det er den n�ste ledige plads. 
       */
      *(b->logbuf + b->pos) = 0;

      /* Navnet til log-filen genereres vha. log-bufferens navn og stationens
	 nummer */
      name = (char *) malloc (sizeof(char) * (strlen(b->name) + 10));
      sprintf(name, "%s.log.%d", b->name, ThisStation);
      fp = fopen(name, "w");

      /* Tidsstempel af log-filen */
      systime = time(NULL);

      fprintf(fp, "LogBuffer: [%s], station: [%d].\n", b->name, ThisStation);
      fprintf(fp, "LogBuffer created %s", asctime(localtime(&systime)));
      fprintf(fp, "Error-frequency: %1.3f\n", GC.ErrorFreq / 1000.0);
      fprintf(fp, "Size of log-data (in bytes): %d\n", b->pos);
      fprintf(fp, "----------------------------------------------------\n");

      /* Selve bufferen udskrives */
      fwrite((char *)b->logbuf, sizeof(char), b->pos, fp);

      fprintf(fp, "----------------------------------------------------\n");
      fprintf(fp, ">> end of log. Dataframe statistic for this station:\n");
      fprintf(fp, "Dataframes sent:\t\t\t%5d.\nReceived:\t\t\t\t%5d.\n",
	      GC.StationStat.frame_sent, GC.StationStat.frame_recv);
      fprintf(fp, "Lost en route to station:\t\t%5d.\nDuplicates:\t\t\t\t%5d.\n",
	      GC.StationStat.frame_lost, GC.StationStat.frame_dupl);
      fprintf(fp, "Dataframes still in buffer:\t\t%5d.\nOverwritten in buffer:\t\t\t%5d.\n",
	      GC.InData.size, GC.StationStat.frame_eras);
      fprintf(fp, "Dataframes delivered via \'FromSubnet\':\t%5d.\n",
	      GC.StationStat.frame_delv);

      fclose(fp);
    }

  UNLOCK( &b->lock , PrintLog );
  return;
}

/*
 * udskriver alle buffere indsat i fifok�en, via InitalizeLB.
 */
void FlushAllLog(void)
{
  FifoQueueEntry e;

  /*
   * For at kunne standse program udf�rslen. SYSTEMOK kan ikke bruges, fordi
   * FlushAllLog skal k�re videre under b�de tilstand YES og STOPPING
   */
  LOCK( &SC.systemlock, FlushAllLog );
  if (SC.ready == NO)
    pthread_cond_wait( &SC.systemsignal, &SC.systemlock );
  UNLOCK( &SC.systemlock, FlushAllLog );

  LOCK( &BC.lock , FlushAllLog );

  while ( !EmptyFQ( BC.BufQ ) )
    {
      e = DequeueFQ( BC.BufQ );
      PrintLog( (LogBuf) ValueOfFQE( e ) );
      DeleteFQE( e );
    }
  DeleteFQ( BC.BufQ );
  UNLOCK( &BC.lock , FlushAllLog );
  return;
} 

/*
 * henter processens navn (hvis muligt)
 */
char *GetProcessName(void)
{
  /* Systemmet k�rer ikke endnu */
  if (SC.ready != YES)
    return NULL;

  /* udskriv navnet */
  return GC.ThArr[*((int *)pthread_getspecific( id ))]->name;
}
