/* 	$Id: network.c,v 1.15 1999/09/06 16:17:16 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Dette program skal bruges til at starte en simulering af et 
 * netværk. Brugeren skal selv designe de enkelte stationer som placeres i et
 * seperat program. 
 *
 * Steffen Schmidt.
 */
static char vcid[]  = "$RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $";
/*
static char rcsid[] = "$Id: network.c,v 1.15 1999/09/06 16:17:16 dim Exp $";
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "network.macro"
#include "network.type"

#define _NETWORK_INC_        /* for at hente data-pakke formatet   */
#include "subnet.h"
#include "subnet.type"
#undef _NETWORK_INC_


/*
 * Globale variable:
 */
int CTRLCFLAG = FALSE;       /* Flag til at afgøre ctrl-c ved log_info.
				ændres i interrupt_handler */
int READYFLAG = NOTOK;       /* Flag der bruges i interrupt_handler. Afgør om
				stationerne er klar til at modtage stop signal. */
int RUNFLAG   = OK;          /* Flag til at afgøre om stationen er igang med
				at afslutte */
int Stations, ErrorFreq, Xterm;
char StName[30];
SocketControl SCP;
FILE *fp;
pid_t new_grpid = 0;
jmp_buf env;

/*
 * Funktioner:
 */

/*
 * timersignal_handler:
 *
 * signalhandler til timeout signal.
 */
void timersignal_handler(int sig)
{
  printf("\n\a>> Fatal error - unable to start all stations within %d sec. Program arborted\n", (Xterm ? XTIMEOUT : TIMEOUT));

  close(SCP[0].sock);             /* lukker socket  */
  unlink(SCP[0].name.sun_path);

  exit(1);
}

/*
 * interupt_handler:
 *
 * signalhandler til interupt signal.
 */
void interupt_handler(int sig)
{
  int i;
  char msg[64];
  BufCtrlStruct buffer = {ctrl_c, 0, 0};

  signal(sig, SIG_IGN);      /* ignorer ctrl-c */

  /* 
   * Er processerne klar til et et stopsignal?
   */
  if (READYFLAG == NOTOK) /* Nej, de skal slås ned */
    {
      killpg(new_grpid, SIGKILL);
      printf("\n\a>> network: ctrl-C received. Setup aborted\n");
      exit(0);
    }
  else                    /* Ja, der kan sendes set stopsignal via sockets */
    {
      /* sendet stopsignal til stationernes data-sockets  */
      for (i = 1; i <= Stations; i++)
	if (SCP[i].active)    /* Sender kun til de stationer der er meldt aktive */

	  if (sendto( SCP[i].sock, (char *)&buffer,
		      sizeof(BufCtrlStruct), 0,
		      (struct sockaddr *)&SCP[i].name,
		      sizeof(struct sockaddr_un)) < 0 )
	    {
	      sprintf(msg, "Sending ctrl-C message to Station %d", i);
	      perror(msg);
	    }
      
      printf("\a>> network: ctrl-C received.\n");
      RUNFLAG   = NOTOK;  /* nu skal vi til at lukke */
      CTRLCFLAG = TRUE;   /* ctrl-c er anvendt       */
      longjmp(env, 1);
    }
}

/*
 * usage:
 *
 * Udskriver hvordan programmet skal bruges, og afslutter.
 */
void usage(void)
{
  printf("\n\nUsage:\tnetwork [options]\nOptions:\n");
  printf("\t-h\t\tPrint this message.\n");
  printf("\t-v\t\tPrint the version number.\n");
  printf("\t-p<name>\tThe name of the networkstation implementation.\n");
  printf("\t-n<number>\tThe number of stations in the network.\n");
  printf("\t-e<number>\tThe errorfrequency in thousands for the network.\n");
  printf("\t-a <arg1 ..>\tAn argumentlist to be given to the networkstation.\n"); 
  printf("\t\t\tEverything that follows -a is given as argument.\n");
  printf("\t-x\t\tStart the simulation without a X-term window\n");
  printf("\t\t\tfor each station.\n\n");
  exit(0);
}

/*
 * print_version:
 *
 * Udskriver versionen af programmet, og afslutter.
 */
void print_version(void)
{
  int x,y,z;
  int day;
  char time[16], month[16], year[16], dum[32];

  printf("\nnetwork, for subnet");

  if (sscanf(vcid, "$RelId: %s %s Version %d.%d (#%d) %s %d %s %s %s",
	     dum, dum, &x, &y, &z, month, &day, time, dum, year)
      == 10)
    {
      printf(", version %d.%d.\n", x, y);
      printf("Subnet source last updated %s %d %s, at %s",
	     month, day, year, time);
    }
  printf(".\nCompiled %s, at %s.\n", __DATE__, __TIME__);
  printf("\nReport bugs to <dim@imada.sdu.dk>.\n");
  exit(0);
}    

/*
 * get_args:
 *
 * Funktionen tager argumenlisten der er givet til programmet, checker den og 
 * sørger for at de oplysninger der er nødvendige er til stede. Dvs. spørger
 * brugeren om de informationer der ikke er givet via argumentlisten.
 */
void get_args(int *argc, char *argv[])
{
  int arg_prgm, arg_error, arg_number, FLAG = TRUE;
  FILE *testfp;

  /*
   * henter argumenter fra argument listen.
   */

  Xterm = OK;  /* som udgangspunkt startes programmerne op via Xterm. */
  arg_prgm = arg_error = arg_number = NOTOK;

  /*
   * Løber igennem argumenterne givet til programmet. Standser når der ikke
   * er flere argumenter, eller mødes en 'option' uden '-', eller hvis FLAG
   * sættes til FALSE (dvs. resten af argumenterne leveres til brugerprogrammet).
   */

  while ((*argc > 1) && (argv[1][0]) == '-' && FLAG) {
    /*
     * argv[1][1] er den egentlige 'option'
     */
    
    switch (argv[1][1]) 
      {
      case 'h': /* udskriv brug. */
	usage();
	break;

      case 'v': /* udskriv versionsnummer. */
	print_version();
	break;

      case 'a': /* argumentliste til brugerprogrammet. */
	FLAG = FALSE;
	break;

      case 'p': /* Navnet på 'stationen' der skal køre i netværket. */
	/* Test om det er et gyldigt navn. */
	if ((testfp = fopen( &argv[1][2], "r")) != NULL )
	  {
	    arg_prgm = OK;
	    sprintf(StName, "%s", &argv[1][2] );
	    fclose(testfp);
	  }
	break;

      case 'n': /* antallet af stationer i netværket. */
	Stations = atoi( &argv[1][2] );
	if (( MINSTATION <= Stations ) && ( Stations <= MAXSTATION ))
	  arg_number = OK;

	break;

      case 'e':	/* fejlfrekvens. */
	ErrorFreq = atoi( &argv[1][2] );
	if (( 0 <= ErrorFreq ) && ( ErrorFreq <= MAXERRORFREQ ))
	  arg_error = OK;
	
	break;

      case 'x': /* programmerne startes IKKE op via Xterm. */
	Xterm = NO;
	break;

      default:	/* ikke nogen i øjeblikket. */
        break;
      }
    
    /* argumentlisten rykkes en frem, og tælleren sættes en ned: */
    argv++; (*argc)--;
  }  /* end-switch */      

  /*
   * De argumenter som ikke var brugbare fra inputlinien fremskaffes nu interaktivt
   * fra brugeren.
   */
  while (!arg_prgm)  /* så længe der ikke er et brugbart navn... */
    {
      printf(">> Enter programname for NetworkStation implementation: ");
      if(scanf("%s", StName) < 1)
	;
      fgetc(stdin);

      if ((testfp = fopen( StName, "r")) != NULL )
	{
	  arg_prgm = OK;
	  fclose(testfp);
	}
      else
	printf("\a-- Illegal stationname!\n");
    }

  if (!arg_number)     /* antallet af stationer i netværket */
    {
      printf(">> Enter number of stations: ");
      if(scanf("%d", &Stations) < 1)

      fgetc(stdin);
  
      if (Stations < MINSTATION)
	{
	  printf("\a\twarning! network: too few stations set. STATIONS = %d.\n",
		 MINSTATION);
	  Stations = MINSTATION;
	}
      else if (Stations > MAXSTATION) 
	{
	  printf("\a\twarning! network: too many stations set. STATIONS = %d.\n",
		 MAXSTATION);
	  Stations = MAXSTATION;
	}
    }

  if (!arg_error)      /* fejl-frekvensen sættes */
    {
      printf(">> Enter error-frequency (in thousandth): ");
      if(scanf("%d", &ErrorFreq) < 1)
	;
      fgetc(stdin);
  
      if (ErrorFreq < 0)
	{
	  printf("\a\twarning! network: too low frequency set. ErrorFreq = 0.\n");
	  ErrorFreq = 0;
	}
      else if (ErrorFreq > MAXERRORFREQ) 
	{
	  printf("\a\twarning! network: too high frequency set. ErrorFreq = %d .\n",
		 MAXERRORFREQ);
	  ErrorFreq = MAXERRORFREQ;
	}
    }

  if ((*argc > 1) && FLAG )
    *argc = 0;              /* resten er ubruglige argumenter */
  else
    (*argc)--;
}

/*
 * socket_setup:
 *
 * Funktionen sætter programmets socket op, og knytter navne til netværkets
 * øvrige sockets.
 *
 * Vedr. navngivning af sockets: "CtrlStat_#_Socket",  # er nummeret på
 * stationen i netværket. "network" har selv en socket "Station_0_Socket".
 */
void socket_setup(void)
{
  int i;

  /* allokerer plads til socketcontrol.
     NB! +1 for at index passer med stations # */
  SCP = (SocketControl) malloc (sizeof(SocketControlStruct) * (Stations + 1));
  
  /* CtrlStat_#_Sockets og Station_#_Sockets */
  if ((SCP[0].sock = socket( AF_UNIX, SOCK_DGRAM, 0 )) < 0) {
    perror("opening Station_0_Socket"); exit(1); }

  SCP[0].name.sun_family = AF_UNIX;
  sprintf(SCP[0].name.sun_path, "Station_0_Socket");
    
  for (i = 1; i <= Stations; i++)
    {
      if ((SCP[i].sock = socket( AF_UNIX, SOCK_DGRAM, 0 )) < 0)
	{
          perror(">> opening ctrl-socket");
	  free(SCP);
	  exit(1);
	}

      SCP[i].name.sun_family = AF_UNIX;
      sprintf(SCP[i].name.sun_path, "CtrlStat_%d_Socket", i);
    }

  /*
   * Knytning af egen socket, så der er klar til modtage "klar-signaler", og
   * derefter et "stop-signal".
   */
  if (write(1,".",1) < 0)
    perror(">> error writing to own socket, unable to receive 'ready signals'");
  if (bind( SCP[0].sock, (struct sockaddr *)&SCP[0].name, 
	    sizeof(struct sockaddr_un)))
    {
      /* Navnet var allerede bundet til en socket. Bindingen prøves fjernet */
      if (errno == EADDRINUSE)
	{
	  unlink( SCP[0].name.sun_path );

	  /* der prøves atter at knytte navn til socket */
	  if (bind( SCP[0].sock, (struct sockaddr *)&SCP[0].name,
		    sizeof(struct sockaddr_un)))
	    {
	      perror(">> Binding name to own socket, unable to unlink old name");
	      free(SCP);
	      exit(1);
	    }
	}
      else
	{
	  perror(">> Binding name to own socket");
	  free(SCP);
	  exit(1);
	}
    }
}

/*
 * start_stations:
 *
 * Funktionen starter netværksstationerne op.
 */
void start_stations(int argc, char *argv[])
{
  pid_t pidloop;
  int i, j, n;
  char **arglist = (char **)malloc(sizeof(char *) * ((Xterm?XARGC:ARGC)+argc)); 
  memset(arglist, '\0', sizeof(char *) * ((Xterm?XARGC:ARGC)+argc));

  if ((new_grpid = fork()) < 0)
    perror(">> fork'ing to get new groupid");
      
  if (new_grpid == 0) /* child */
    {
      /* sætter et nyt gruppe id, så xterm-vinduerne ikke dør ved ctrl-c */
#ifdef __FreeBSD__
      if (setpgid(0,0) < 0)
#else
      if (setpgrp() < 0)
#endif
	perror(">> Setting new groupid");
      /* allokerer plads til argumenter i argument array'et */
      for (i = 0; i < ((Xterm?XARGC:ARGC) + argc); i++)
	arglist[i] = (char *) malloc (sizeof(char) * 32);

      /* starter stationerne op */
      for (i = 0; i < Stations; i++)
        {
	  if ((pidloop = fork()) < 0)
	    perror(">> fork'ing to start new stations");

	  if (pidloop == 0) /* child i inderste fork */
	    {
	      j = 0;
	      if (Xterm)  /* specielt til x-term */
		{
		  sprintf( arglist[j++], "xterm");
		  sprintf( arglist[j++], "-fn");
		  sprintf( arglist[j++], "%s", FONT);
		  sprintf( arglist[j++], "-geometry");
		  sprintf( arglist[j++], "%s+%d+%d", GEOM, 10 + 5*i, 100+45*i);
		  sprintf( arglist[j++], "-title");
		  sprintf( arglist[j++], "Station_%d", i+1);
		  sprintf( arglist[j++], "-e");
		}
	      /* fælles... */
	      sprintf( arglist[j++], "./%s", StName);
	      sprintf( arglist[j++], "%d", i+1);
	      
	      /* evt. ekstra argumenter kopieres*/
	      for (n = 0; n < argc; n++)
		sprintf( arglist[j++], "%s", argv[n]);
	      
	      /* SKAL afsluttes med NULL */
	      arglist[j] = NULL;

	      /* start station: */
	      execvp(arglist[0], arglist);
	    }
	  if (write(1,".",1) < 0)
            perror(">> Unable to write to own socket");
        } /* parent fortsætter i 'for' loop */
        for (i = 0; i < ((Xterm?XARGC:ARGC) + argc); i++)
          free(arglist[i]);
        free(arglist);
      /* child af yderste fork afsluttes */
      exit(0);
    }

  for (i = 0; i < ((Xterm?XARGC:ARGC) + argc); i++)
    free(arglist[i]);
  free(arglist);
  /* parent af yderste fork() fortsætter her */

  /* signalhandler for ctrl-C */
  signal(SIGINT, interupt_handler);
}

/*
 * open_logfile:
 *
 * Funktionen åbner en logfil til skrivning af synkronisteret log-informationer.
 */
void open_logfile(void)
{
  char msg[80]; 
  time_t systime;
  struct tm tmtime;
 
  sprintf( msg, "%s.network.log", StName );

  if ( (fp = fopen( msg, "w")) == NULL )
    perror(">> opening 'network.log'");

  /* Tidsstempel af log-filen */
  systime = time(NULL);
  tmtime  = *localtime(&systime);

  /* lidt "tiden g�r" udskrift */
  if (write(1,".",1) < 0)
    perror(">> Unable to write to own socket");

  /* Laver et "hoved" p� log-filen */
  strftime(msg, sizeof(msg), " %d/%m-%y, at %H:%M:%S\n", &tmtime );
  fprintf(fp,"Network log. Synchronized log-informations:\n");
  fprintf(fp,"Log opened:");
  fprintf(fp,"%s",msg);
  /* fprintf(fp,msg); */
}

/*
 * get_ready:
 *
 * Funktionen venter på at få besked fra alle de stationer der blev startet op
 * om de er aktive eller ej.
 */
void get_ready(void)
{
  int i, q;
  pid_t pid;
  char dummy[32];
  char buffer[SETUPINFOSIZE];
  int semaphore = Stations; /* hvor mange stationer skal der ventes på svar fra */
  int FLAG = FALSE;         /* er der nogen stationer aktive i det hele taget?  */

  /* Løkken kører indtil der er besked fra alle stationer. */
  while(semaphore)
    {
      if (read( SCP[0].sock, buffer, SETUPINFOSIZE) < 0)
	{
	  perror(">> reading ready or not signals");
	  printf(">> errno: %d.\n", errno);
	}
      else
	{
	  /* henter informationer fra bufferen */
	  sscanf(buffer, "%d %s %d", &i, dummy, &q);
          pid = (pid_t)q;
	  if (NULL == strstr(buffer, "NONE"))
	    { /* hvis NONE ikke forekommer, s� er stationen aktiv */ 
	      FLAG          = TRUE;
	      SCP[i].active = OK;
	    }
	  else
	    SCP[i].active = NOTOK;
	  
	  SCP[i].pid = pid; 

	  for (i = 0; i < SETUPINFOSIZE; buffer[i++] = 0);

	  if (write(1,".",1) < 0) /* lidt udskrift */
             perror(">> unable to write to own socket");
	  semaphore--;
	}
    }
  alarm(0); /* slår alarmen (timeren) fra */
  
  /* Er der stationer der er aktive? */ 
  if (FLAG)
    return;
  else
    {
      printf("\n\a>> None of the %d stations is active. Setup aborted.\n",
	     Stations);
      exit(1);
    }
}

/*
 * transfer_info:
 *
 * Funktionen sender oplysninger til alle netværketsstaioner om:
 * Om den enkelte station er startet op i et x-term vindue eller ej, antallet 
 * af stationer alt i alt, hvilke der er aktive og hvad fejlfrekvensen er.
 *
 * Informationerne sendes som et array af char, der har formatet:
 * {station startet i x-term vindue ja/nej (1/0),
 *  antal stationer,
 *  station 1 -  aktiv/ikke aktiv,
 *  ... ,
 *  antal 1000'er i fejlfrekvens.
 *  antal 100'er  i fejlfrekvens.
 *  antal 10'er   i fejlfrekvens.
 *  antal 1'er    i fejlfrekvens.}
 */
void transfer_info(void)
{
  int i;
  char msg[80];
  char buffer[SETUPINFOSIZE];

  memset(msg, '\0', 80);
  memset(buffer, '\0', SETUPINFOSIZE);

  /* er stationen startet i et x-term vindue */
  buffer[0] = (char) Xterm;

  /* det samlede antal gøres klar til overførsel */
  buffer[1] = (char) Stations;  

  /* numrene på de aktive stationer gøres klar til overførsel */
  printf(".\n");
  for (i = 1; i <= Stations; i++)
    {
      printf(">> Station %d, (pid: %d) - %s\n",
	     i, (int)SCP[i].pid, ( SCP[i].active ? "Active" : "Not active" ));
      buffer[i+1] = SCP[i].active;
    }

  /* skrives i logbufferen */
  fprintf(fp, "Error-frequency: %1.3f\nActive stations:", ErrorFreq / 1000.0); 

  /* fejlfrekvensen gøres klar til overførsel */
  buffer[++i] = ErrorFreq / 1000; ErrorFreq %= 1000;
  buffer[++i] = ErrorFreq / 100 ; ErrorFreq %= 100;
  buffer[++i] = ErrorFreq / 10;
  buffer[++i] = ErrorFreq % 10;


  /*
   * Når stationerne modtager disse data, så virker det som et GO signal. Er
   * stationerne startet op i x-term vinduer, er der brug for en pause, så
   * brugeren kan placerer sine xterm vinduer.
   */
  if (Xterm)
    {
      printf(">> Press enter when ready!\n");
      while ( fgetc(stdin) != 10 );
    }

  /*
   * Der sendes et GO til alle aktive stationer i form af information om
   * netværkets opsætning. Der sendes til CtrlStat_#_Socket!!
   */ 
  for (i = 1; i <= Stations; i++)
    if (SCP[i].active)    /* Sender kun til de stationer der er meldt aktive */
      {
	if (sendto( SCP[i].sock, buffer, SETUPINFOSIZE, 0, 
		    (struct sockaddr *)&SCP[i].name,
		    sizeof(struct sockaddr_un)) < 0 )
	  {
	    sprintf(msg, ">> Sending GO! message to Station %d", i);
	    perror(msg);
	  }
	else
	  {
	    printf(">> Sending GO! to station %d\n", i);
	    fprintf(fp, " %d", i);
	  }
      }
  if (!RUNFLAG) printf(">> setup aborted\n");
}

/*
 * receive_log:
 *
 * Funktionen modtager synkroniseret log informationer.
 */
void receive_log(void)
{
  register int i, length, pos, cnt;
  int num, MSGFLAG = TRUE, SETJMPFLAG = FALSE;
  char  *buffer, msg[80];
  BufLogStruct inbuffer;
  fd_set ready;
  struct timeval tv;

  tv.tv_sec  = 1;       /* 1.5 sek. Den tid select højest venter på data */
  tv.tv_usec = 500000;  /* på network's socket under afslutning af prgm. */

  FD_ZERO( &ready );
  FD_SET( SCP[0].sock, &ready );

  num = 1; pos = 0; cnt = 0; 

  buffer   = (char *) malloc (sizeof(char) * CHUNKSIZE * num );

  fprintf(fp,"\n-------------------------------------------\n");
  fprintf(fp,"[[ (log-number) ]]< (source) >: (data)\n");
  fprintf(fp,"-------------------------------------------\n");


  /* log-data opsamling: */
  while(1)
    {
      /* jmp_buf sættes kun en gang inde i while løkken */
      if (SETJMPFLAG == FALSE)
	{
	  setjmp(env);

	  /* Sætter READYFLAG: stationerne er klar til at modtage et stop-
	   * signal, og env er sat, så den kan kaldes fra interupt_handler. */
	  READYFLAG  = OK;
	  SETJMPFLAG = TRUE;
	}

      if (RUNFLAG)    /* systemer er kørende, dvs. normal 'read'   */
	{
	  if (read( SCP[0].sock, (char *)&inbuffer,
		    sizeof(BufLogStruct)) < 0)
	    if (errno != EINTR)             /* forskellig fra ctrl-C */
	      perror(">> Receiving datagram");
	}
      else            /* systemet er ved at stoppe, check om der er data
			 på networks socket vha. system kaldet 'select'. */

	{
	  if (MSGFLAG && cnt > 0)
	    {
	      printf(">> Collecting rest of SyncLog data.\n");
	      MSGFLAG = FALSE;
	    }
	  
	  /* checker om der er data på socketen */
	  if (select( SCP[0].sock + 1, &ready, NULL, NULL, &tv ) < 0 )
	    perror(">> Checking for data on socket");
	  
	  /* er der data? */
	  if (FD_ISSET( SCP[0].sock, &ready )) 
	    { /* ja, l�s det */
	      if (read( SCP[0].sock, (char *)&inbuffer, 
			sizeof(BufLogStruct)) < 0)
		perror(">> Receiving datagram");
	    }
	  else
	    { /* nej, luk */
	      shutdown(SCP[0].sock, 2);   /* lukker socket  */

	      if (cnt > 0)   /* er der skrevet log info? */
		{
		  fwrite((char *)buffer, sizeof(char), pos, fp);
		  /* har ctrl-c v�ret anvendt? */
		  if (CTRLCFLAG)
		    fprintf(fp, "\n>> terminated by ctrl-C");
		  fprintf( fp, "\n>> end of log.\n");
		  fclose(fp);             /* lukker log-fil */
		}
	      else            /* ingen information, så fjern filen.  */
		{
		  fclose(fp);
		  sprintf( msg, "%s.network.log", StName );
		  remove( msg );
		}

	      unlink(SCP[0].name.sun_path);
	      printf(">> Done!\n\n");
	      free(buffer);
	      exit(0);
	    }
	}

      /*
       * Typen af den modtagne datapakke afgører hvad der skal ske. Der er to
       * mulighedder:
       *
       * 1) "stop":    Programudførslen stoppes, og logfilen skrives (hvis der er
       *               modtaget noget log-data.)
       * 2) "data":    Logdata. Dette skrives i første omgang til hukommelsen, og
       *               først når programmet stopper skrives det til en fil.
       */

      if ( inbuffer.type == (unsigned char)stop )
	/* så stopper vi! */
	{
	  printf(">> Stop signal received.\n");

	  if (cnt > 0)                    /* er der skrevet log informationer? */
	    {
	      RUNFLAG = NOTOK;
	    }
	  else
	    {
	      shutdown(SCP[0].sock, 2);   /* lukker socket  */
	      sprintf( msg, "%s.network.log", StName );
	      fclose(fp);                 /* hvis der ikke er skrevet nogen log- */
	      remove( msg );              /* information, så fjernes filen.      */
	      unlink(SCP[0].name.sun_path);
	      free(SCP);
	      printf(">> Done!\n\n");
	      free(buffer);
	      exit(0);
	    }
	  
	}
      
      if ( inbuffer.type == (unsigned char)data )
	{
	  /*
	   * Der logges til hukommelsen i første omgang. Der allokeres
	   * CHUNKSIZE bytes ad gangen.
	   */
 	  sprintf(msg, "[[%d]]<%d>: ", ++cnt, inbuffer.station);
	  length = strlen(msg);

	  if ( pos + length >= num * CHUNKSIZE )
	    buffer = (char *)realloc(buffer, CHUNKSIZE * sizeof(char) * (++num));

	  memcpy( (buffer + pos), msg, length);
	  pos += length;

	  if ( pos + inbuffer.size >= num * CHUNKSIZE )
	    buffer = (char *)realloc(buffer, CHUNKSIZE * sizeof(char) * (++num));
	  
	  memcpy( (buffer + pos), inbuffer.data, inbuffer.size);
	  pos += inbuffer.size;

	  /* nulstiller buffer istedet for at allokerer ny hukommelse */
	  for (i = 0; i < inbuffer.size;  inbuffer.data[i++] = 0);
	}
    }
    free(buffer);
}

/*********************************************************************************
 * Hovedprogram:
 *********************************************************************************
 *
 */

int main(int argc, char *argv[])
{
  int i, tempargc = argc;

  /* Check at alle argumenter er der */
  get_args(&argc, argv);

  /* rykker argv, så de godkendte argumenter fjernes */
  for (;tempargc > argc; tempargc--,argv++);

  /* udskriver en eventuel argumentliste til netværksstationerne */
  if (argc > 0)
    {
      printf(">> Argumentlist for all networkstations: \"");
      for (i = 0; i < argc; i++)
	printf("%s%s", argv[i], (i+1==argc?"":" "));
     
      printf("\"\n");
    }

  /* Laver lidt udskrift, så brugeren kan se programmet kører.. */
  if (write(1,">> Wait", 7) < 0)
    perror(">> unable to write to own socket");

  /* Processerne kommunikerer via sockets. Sæt disse op. */
  socket_setup();

  /* Stationerne startes. */
  start_stations(argc, argv);
  
  /*
   * Start timer for korrekt opstart. Der benyttes forskellig længde af
   * timeout, alt efter om stationerne er startet via Xterm eller ej.
   */
  signal(SIGALRM, timersignal_handler); /* signalhandler til timeout signal */

  if (Xterm)
    /* venter 'XTIMEOUT' antal sekunder inden der sendes et timeout signal */
    alarm(XTIMEOUT);
  else
    /* venter 'TIMEOUT' antal sekunder inden der sendes et timeout signal */
    alarm(TIMEOUT);
				 
  /* Åbner en fil, så der er klar til at blive skrevet log-informationer. */
  open_logfile();

  /* Venter på READY fra alle stationer. */
  get_ready();
  
  /*
   * Informationer om antallet af stationer, hvilke der er aktive og fejlfrekvens
   * for netværket skal overføres til alle de aktive stationer.
   */
  transfer_info();

  /* venter på log-informationer, og/eller et stopsignal; */
  receive_log();
 
  exit(0);
}
