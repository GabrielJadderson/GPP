/*
 * Reliable data transfer between two stations
 *
 * Author: Jacob Aae Mikkelsen.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rdt.h"
#include "subnetsupport.h"
#include "subnet.h"
#include "fifoqueue.h"
#include "debug.h"

/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

#define MAX_SEQ 127        /* should be 2^n - 1 */
#define NR_BUFS 4

#define NUM_MAX_NEIGHBOURS 4

/* Globale variable */

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */
log_type LogStyle;         /* Hvilken slags log skal systemet føre            */
boolean network_layer_enabled;

LogBuf mylog;                /* logbufferen                                     */

FifoQueue from_network_layer_queue;           		/* Queue for data from network layer */
FifoQueue for_network_layer_queue;    /* Queue for data for the network layer */

mlock_t *network_layer_lock;
mlock_t *write_lock;

neighbour neighbours[NUM_MAX_NEIGHBOURS];
//int ack_timer_id; // [PJ] This will be nuked at some point
//int ack_timer_ids[NUM_MAX_NEIGHBOURS];
//int timer_ids[NR_BUFS];
//int timer_ids2D[NR_BUFS];
//boolean no_nak = false; /* no nak has been sent yet */

static boolean between(seq_nr a, seq_nr b, seq_nr c)
{
	boolean x = (a <= b) && (b < c);
	boolean y = (c < a) && (a <= b);
	boolean z = (b < c) && (c < a);
	// TODO Omskriv så det er til at fatte!
	logLine(debug, "a==%d, b=%d, c=%d, x=%d, y=%d, z=%d\n", a,b,c,x,y,z);

    return x || y || z;
}

/* Copies package content to buffer, ensuring it has a string end character. */
void packet_to_string(packet* data, char* buffer)
{
	strncpy ( buffer, (char*) data->data, MAX_PKT );
	buffer[MAX_PKT] = '\0';

}

static void send_frame(frame_kind fk, seq_nr frame_nr, seq_nr frame_expected, packet buffer[], neighbourid recipient)
{
    /* Construct and send a data, ack, or nak frame. */
    frame s;        /* scratch variable */

    s.kind = fk;        /* kind == data, ack, or nak */
    if (fk == DATA)
    {
    	s.info = buffer[frame_nr % NR_BUFS];
    }
    s.seq = frame_nr;        /* only meaningful for data frames */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
//            	logLine(succes, "send_frame: %d, %d, %d\n", frame_expected, s.ack, s.kind);
    if (fk == NAK)
    {
    	neighbours[0].no_nak = false;        /* one nak per frame, please */
    }
    //logLine(succes, "Sending frame to physical layer for recipient: %d\n", recipient);
    to_physical_layer(&s, neighbours[recipient].stationID);        /* transmit the frame */
    if (fk == DATA)
    {
    	start_timer(recipient, frame_nr); //TODO
    }
    stop_ack_timer(0);        /* no need for separate ack frame */
}

#include "rdt_fakeNetworkLayers.c"

void log_event_received(long int event) {
	char *event_name;
	switch(event) {
		case 1:
			event_name = "frame_arrival";
			break;
		case 2:
			event_name = "timeout";
			break;
		case 4:
			event_name = "network_layer_allowed_to_send";
			break;
		case 8:
			event_name = "network_layer_ready";
			break;
		case 16:
			event_name = "data_for_network_layer";
			break;
		default:
			event_name = "unknown";
			break;
	}
	logLine(trace, "Event received %s\n", event_name);

}

void selective_repeat() {
    
    //seq_nr ack_expected;              // lower edge of sender's window
    //seq_nr next_frame_to_send;        // upper edge of sender's window + 1
    //seq_nr frame_expected;            // lower edge of receiver's window
    //seq_nr too_far;                   // upper edge of receiver's window + 1
    //int i;                            // index into buffer pool
    //frame r;                          // scratch variable
    //packet out_buf[NR_BUFS];          // buffers for the outbound stream
    //packet in_buf[NR_BUFS];           // buffers for the inbound stream
    //boolean arrived[NR_BUFS];         // inbound bit map
    /*seq_nr nbuffered;                 // how many output buffers currently used
    */
    frame r;                          // scratch variable //[PJ] Moved out here to block comment the above as a single unit.
    
    int currentNeighbour = 0; //[PJ] Each time this is used, it should be set that use
    neighbour_SR_Data neighbourData[NUM_MAX_NEIGHBOURS];
    event_t event;
    long int events_we_handle;
    unsigned int timer_id;

    write_lock = malloc(sizeof(mlock_t));
    network_layer_lock = (mlock_t *)malloc(sizeof(mlock_t));

    Init_lock(write_lock);
    Init_lock( network_layer_lock );

    /*
    logLine(succes, "Log Enabled: succes\n");
    logLine(error, "Log Enabled: error\n");
    logLine(warn, "Log Enabled: warn\n");
    logLine(info, "Log Enabled: info\n");
    logLine(debug, "Log Enabled: debug\n");
    logLine(trace, "Log Enabled: trace\n");
    */

    
    enable_network_layer();  // initialize
    //ack_expected = 0;        // next ack expected on the inbound stream
    //next_frame_to_send = 0;        // number of next outgoing frame
    //frame_expected = 0;        // frame number expected
    //too_far = NR_BUFS;        // receiver's upper window + 1
    //nbuffered = 0;        // initially no packets are buffered
    

    logLine(trace, "Starting selective repeat %d\n", ThisStation);

    /*for (int i = 0; i < NR_BUFS; i++) {
      arrived[i] = false;
      //timer_ids[i] = -1;
    }*/
    //ack_timer_id = -1;
    
    // Set all the timers to be not set.
    for (int i = 0; i < NUM_MAX_NEIGHBOURS; i++) {
      //ack_timer_ids[i] = -1;
      neighbours[i].ack_timer_id = -1;
      
      for (int j = 0; j < NR_BUFS; j++) {
        neighbours[i].timer_ids[j] = -1;
      }
      
      init_neighbour_SR_Data(&neighbourData[i]);
      //logLine(succes, "%d, %d, %d, %d, %d\n", neighbourData[i].ack_expected, neighbourData[i].next_frame_to_send, neighbourData[i].frame_expected, neighbourData[i].too_far, neighbourData[i].nbuffered);
    }
    //logLine(succes, "%d, %d, %d, %d, %d\n", neighbourData[0].ack_expected, neighbourData[0].next_frame_to_send, neighbourData[0].frame_expected, neighbourData[0].too_far, neighbourData[0].nbuffered);


    events_we_handle = frame_arrival | timeout | network_layer_ready;


    /*
    // If you are in doubt how the event numbers should be, comment in this, and you will find out.
    printf("%#010x\n", 1);
    printf("%#010x\n", 2);
    printf("%#010x\n", 4);
    printf("%#010x\n", 8);
    printf("%#010x\n", 16);
    printf("%#010x\n", 32);
    printf("%#010x\n", 64);
    printf("%#010x\n", 128);
    printf("%#010x\n", 256);
    printf("%#010x\n", 512);
    printf("%#010x\n", 1024);
	*/

    while (true) {
        // Wait for any of these events

        Wait(&event, events_we_handle);
        log_event_received(event.type);

        switch(event.type) {
            case network_layer_ready:        /* accept, save, and transmit a new frame */
            	logLine(trace, "Network layer delivers frame - lets send it\n");
                    
                    currentNeighbour = event.msg;
                    
	            neighbourData[currentNeighbour].nbuffered = neighbourData[currentNeighbour].nbuffered + 1;        /* expand the window */
	            from_network_layer(&neighbourData[currentNeighbour].out_buf[neighbourData[currentNeighbour].next_frame_to_send % NR_BUFS]); /* fetch new packet */
	            send_frame(DATA, neighbourData[currentNeighbour].next_frame_to_send, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);        /* transmit the frame */
	            inc(neighbourData[currentNeighbour].next_frame_to_send);        /* advance upper window edge */
	            break;

	        case frame_arrival:        /* a data or control frame has arrived */
				currentNeighbour = from_physical_layer(&r); //stationID2neighbourindex(from_physical_layer(&r));        /* fetch incoming frame from physical layer */
//            	logLine(succes, "frame_arrival r.ack: %d\n", r.ack);
				if (r.kind == DATA) {
					/* An undamaged frame has arrived. */
					if ((r.seq != neighbourData[currentNeighbour].frame_expected) && neighbours[0].no_nak) {
						send_frame(NAK, 0, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);
					} else {
						start_ack_timer(currentNeighbour);
					}
					if (between(neighbourData[currentNeighbour].frame_expected, r.seq, neighbourData[currentNeighbour].too_far) && (neighbourData[currentNeighbour].arrived[r.seq%NR_BUFS] == false)) {
						/* Frames may be accepted in any order. */
						neighbourData[currentNeighbour].arrived[r.seq % NR_BUFS] = true;        /* mark buffer as full */
						neighbourData[currentNeighbour].in_buf[r.seq % NR_BUFS] = r.info;        /* insert data into buffer */
						while (neighbourData[currentNeighbour].arrived[neighbourData[currentNeighbour].frame_expected % NR_BUFS]) {
							/* Pass frames and advance window. */
							to_network_layer(&neighbourData[currentNeighbour].in_buf[neighbourData[currentNeighbour].frame_expected % NR_BUFS]);
							neighbours[currentNeighbour].no_nak = true;
							neighbourData[currentNeighbour].arrived[neighbourData[currentNeighbour].frame_expected % NR_BUFS] = false;
							inc(neighbourData[currentNeighbour].frame_expected);        /* advance lower edge of receiver's window */
							inc(neighbourData[currentNeighbour].too_far);        /* advance upper edge of receiver's window */
							start_ack_timer(currentNeighbour);        /* to see if (a separate ack is needed */
						}
					}
				}
				if((r.kind==NAK) && between(neighbourData[currentNeighbour].ack_expected,(r.ack+1)%(MAX_SEQ+1),neighbourData[currentNeighbour].next_frame_to_send))	{
					send_frame(DATA, (r.ack+1) % (MAX_SEQ + 1), neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);
				}

				logLine(info, "Are we between so we can advance window? ack_expected=%d, r.ack=%d, next_frame_to_send=%d\n", neighbourData[currentNeighbour].ack_expected, r.ack, neighbourData[currentNeighbour].next_frame_to_send);
				while (between(neighbourData[currentNeighbour].ack_expected, r.ack, neighbourData[currentNeighbour].next_frame_to_send)) {
					logLine(debug, "Advancing window %d\n", neighbourData[currentNeighbour].ack_expected);
					neighbourData[currentNeighbour].nbuffered = neighbourData[currentNeighbour].nbuffered - 1;        		/* handle piggybacked ack */
					//stop_timer(ack_expected % NR_BUFS);     /* frame arrived intact */
					stop_timer(currentNeighbour, neighbourData[currentNeighbour].ack_expected % NR_BUFS);     /* frame arrived intact */ //TODO
					inc(neighbourData[currentNeighbour].ack_expected);        				/* advance lower edge of sender's window */
				}
				break;

	        case timeout: /* Ack timeout or regular timeout*/
	        	// Check if it is the ack_timer
	        	timer_id = event.timer_id;
	        	//logLine(trace, "Timeout with id: %d - acktimer_id is %d\n", timer_id, ack_timer_id);
	        	//logLine(trace, "Timeout with id: %d - acktimer_id is %d\n", timer_id, ack_timer_ids[0]);
	        	logLine(trace, "Timeout with id: %d - acktimer_id is %d\n", timer_id, neighbours[0].ack_timer_id);
	        	//logLine(info, "Message from timer: '%s'\n", (char *) event.msg ); //[PJ] since it is no longer a char*, this won't work.
                        
                        //Figure out which neighbour to resend to.
                        currentNeighbour = ((packetTimerMessage*) event.msg)->neighbour;
                        //logLine(succes, "Timer values: %d, %d\n", ((packetTimerMessage*)event.msg)->k, currentNeighbour);
                        //logLine(succes, "timer stuff: %s\n", (char *) event.msg);

	        	//if( timer_id == ack_timer_id ) { // Ack timer timer out
	        	//if( timer_id == ack_timer_ids[0] ) { // Ack timer timer out
	        	if( timer_id == neighbours[currentNeighbour].ack_timer_id ) { // Ack timer timer out
	        		logLine(debug, "This was an ack-timer timeout. Sending explicit ack.\n");
	        		free(event.msg);
	        		//ack_timer_id = -1; // It is no longer running
	        		//ack_timer_ids[0] = -1; // It is no longer running
	        		neighbours[currentNeighbour].ack_timer_id = -1; // It is no longer running
	        		send_frame(ACK,0,neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);        /* ack timer expired; send ack */
	        	} else {
	        		//int timed_out_seq_nr = atoi( (char *) event.msg );
	        		int timed_out_seq_nr = ((packetTimerMessage *) event.msg)->k;

	        		logLine(debug, "Timeout for frame - need to resend frame %d\n", timed_out_seq_nr);
		        	send_frame(DATA, timed_out_seq_nr, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);
	        	}
	        	break;
	     }

	     if (neighbourData[currentNeighbour].nbuffered < NR_BUFS) {
	         enable_network_layer();
	     } else {
	    	 disable_network_layer();
	     }
	  }
}

void init_neighbour_SR_Data(neighbour_SR_Data *ND) {
  ND->ack_expected = 0;
  ND->next_frame_to_send = 0;
  ND->frame_expected = 0;
  ND->too_far = NR_BUFS;
  ND->nbuffered = 0;
  for (int i = 0; i < NR_BUFS; i++) {
    ND->arrived[i] = false;
  }
}

neighbourid stationID2neighbourindex(int stationID) {
  int i = 0;
  while(neighbours[i].stationID != stationID) {
    i++;
    
    if(i >= NUM_MAX_NEIGHBOURS) {
      logLine(error, "stationID2neighboutindex: unable to find neighbour for station: %d\n", stationID);
      Stop(); //[PJ] Nope!
    }
  }
  
  return i;
}

void enable_network_layer(void) {
	Lock( network_layer_lock );
	logLine(trace, "enabling network layer\n");
	network_layer_enabled = true;
	Signal( network_layer_allowed_to_send, NULL );
	Unlock( network_layer_lock );
}

void disable_network_layer(void){
	Lock( network_layer_lock );
	logLine(trace, "disabling network layer\n");
	network_layer_enabled = false;
	Unlock( network_layer_lock );
}

void from_network_layer(packet *p) {
    FifoQueueEntry e;

	Lock( network_layer_lock );
	e = DequeueFQ( from_network_layer_queue );
    Unlock( network_layer_lock );

	if(!e) {
		logLine(error, "ERROR: We did not receive anything from the queue, like we should have\n");
	} else {
		memcpy(p, (char *)ValueOfFQE( e ), sizeof(packet));
	    free( (void *)ValueOfFQE( e ) );
		DeleteFQE( e );
	}
}


void to_network_layer(packet *p) {
	char * buffer;
    Lock( network_layer_lock );

    buffer = (char *) malloc ( sizeof(char) * (1+MAX_PKT));
    packet_to_string(p, buffer);

    EnqueueFQ( NewFQE( (void *) buffer ), for_network_layer_queue );

    Unlock( network_layer_lock );

    Signal( data_for_network_layer, NULL);
}


void print_frame(frame* s, char *direction) {
	char temp[MAX_PKT+1];

	switch( s->kind ) {
		case ACK:
			logLine(info, "%s: ACK frame. Ack seq_nr=%d\n", direction, s->ack);
			break;
		case NAK:
			logLine(info, "%s: NAK frame. Nak seq_nr=%d\n", direction, s->ack);
			break;
		case DATA:
			packet_to_string(&(s->info), temp);
			logLine(info, "%s: DATA frame [seq=%d, ack=%d, kind=%d, (%s)] \n", direction, s->seq, s->ack, s->kind, temp);
			break;
	}
}

int from_physical_layer(frame *r) {
	r->seq = 0;
	r->kind = DATA;
	r->ack = 0;

	int source, dest, length;

	logLine(trace, "Receiving from subnet in station %d\n", ThisStation);
	FromSubnet(&source, &dest, (char *) r, &length);
	print_frame(r, "received");

	return stationID2neighbourindex(source);
}


void to_physical_layer(frame *s, int target)
{
	/*int send_to;

	if( ThisStation == 1) {
		send_to = 2;
	} else {
		send_to = 1;
	}*/

	print_frame(s, "sending");
        
        //char temp[MAX_PKT+1];
        //packet_to_string(&(s->info), temp);
	//ToSubnet(ThisStation, send_to, (char *) s, sizeof(frame));
	ToSubnet(ThisStation, target, (char *) s, sizeof(frame));
        //logLine(succes, "Trying to send packet to: %d; with contents: %s\n", target, temp);
}


void start_timer(neighbourid neighbour, seq_nr k) {

	//char *msg;
	//msg = (char *) malloc(100*sizeof(char));
	//sprintf(msg, "%d", k); // Save seq_nr in message
        //msg = malloc(sizeof(neighbourid)+sizeof(seq_nr));
        //msg[0] = neighbour;
        //msg[sizeof(neighbourid)] = k;
        
        //logLine(succes, "starting timer with values: %d, %d\n", k, neighbour);
        packetTimerMessage *msg = malloc(sizeof(packetTimerMessage));
        msg->k = k;
        msg->neighbour = neighbour;
        
        //char *msg = "Du Milde Moses!\n"; //[PJ] Just debugging

	//timer_ids[k % NR_BUFS] = SetTimer( frame_timer_timeout_millis, (void *)msg );
	neighbours[neighbour].timer_ids[k % NR_BUFS] = SetTimer( frame_timer_timeout_millis, (void *)msg );
	//logLine(trace, "start_timer for seq_nr=%d timer_ids=[%d, %d, %d, %d] %s\n", k, timer_ids[0], timer_ids[1], timer_ids[2], timer_ids[3], msg);
	logLine(trace, "start_timer for seq_nr=%d timer_ids=[%d, %d, %d, %d] %s\n", k, neighbours[neighbour].timer_ids[0], neighbours[neighbour].timer_ids[1], neighbours[neighbour].timer_ids[2], neighbours[neighbour].timer_ids[3], msg);

}


void stop_timer(neighbourid neighbour, seq_nr k) {
	int timer_id;
	char *msg;

	//timer_id = timer_ids[k];
	timer_id = neighbours[neighbour].timer_ids[k];
	logLine(trace, "stop_timer for seq_nr %d med id=%d\n", k, timer_id);

    if (StopTimer(timer_id, (void *)&msg)) {
    	logLine(trace, "timer %d stoppet. msg: %s \n", timer_id, msg);
        free(msg);
    } else {
    	//logLine(trace, "timer %d kunne ikke stoppes. Måske er den timet ud?timer_ids=[%d, %d, %d, %d] \n", timer_id, timer_ids[0], timer_ids[1], timer_ids[2], timer_ids[3]);
    	logLine(trace, "timer %d kunne ikke stoppes. Måske er den timet ud?timer_ids=[%d, %d, %d, %d] \n", timer_id, neighbours[neighbour].timer_ids[0], neighbours[neighbour].timer_ids[1], neighbours[neighbour].timer_ids[2], neighbours[neighbour].timer_ids[3]);
    }
}


void start_ack_timer(neighbourid neighbour)
{
  //if( ack_timer_id == -1 ) {
  //if( ack_timer_ids[neighbour] == -1 ) {
  if( neighbours[neighbour].ack_timer_id == -1 ) {
    logLine(trace, "Starting ack-timer\n");
    //char *msg;
    //msg = (char *) malloc(100*sizeof(char));
    //strcpy(msg, "Ack-timer");
    packetTimerMessage *msg = malloc(sizeof(packetTimerMessage));
    msg->k = -1;
    msg->neighbour = neighbour;
    //ack_timer_id = SetTimer( act_timer_timeout_millis, (void *)msg );
    //ack_timer_ids[neighbour] = SetTimer( act_timer_timeout_millis, (void *)msg );
    neighbours[neighbour].ack_timer_id = SetTimer( act_timer_timeout_millis, (void *)msg );
    //logLine(debug, "Ack-timer startet med id %d\n", ack_timer_id);
    //logLine(debug, "Ack-timer startet med id %d\n", ack_timer_ids[0]);
    logLine(debug, "Ack-timer startet med id %d\n", neighbours[neighbour].ack_timer_id);
  }
}


void stop_ack_timer(neighbourid neighbour)
{
  char *msg;
  
  logLine(trace, "stop_ack_timer\n");
  //if (StopTimer(ack_timer_id, (void *)&msg)) {
  //if(StopTimer(ack_timer_ids[neighbor], (void*)&msg)) {
  if(StopTimer(neighbours[neighbour].ack_timer_id, (void*)&msg)) {
    //logLine(trace, "timer %d stoppet. msg: %s \n", ack_timer_id, msg);
    //logLine(trace, "timer %d stoppet. msg: %s \n", ack_timer_ids[neighbor], msg);
    logLine(trace, "timer %d stoppet. msg: %s \n", neighbours[neighbour].ack_timer_id, msg);
    free(msg);
  }
  //ack_timer_id = -1;
  //ack_timer_ids[neighbor] = -1;
  neighbours[neighbour].ack_timer_id = -1;
}


extern int global_log_level_limit;
int main(int argc, char *argv[])
{
  StationName = argv[0];
  ThisStation = atoi( argv[1] );

  if (argc == 3) {
    printf("Station %d: arg2 = %s\n", ThisStation, argv[2]);
    global_log_level_limit = atoi(argv[2]);
  } else {
    global_log_level_limit = succes;
  }

  mylog = InitializeLB("mytest");

    LogStyle = synchronized;

    printf( "Starting network simulation\n" );


  /* processerne aktiveres */
  /*ACTIVATE(1, FakeNetworkLayer1);
  ACTIVATE(2, FakeNetworkLayer2);
  ACTIVATE(1, selective_repeat);
  ACTIVATE(2, selective_repeat);
  //ACTIVATE(1, selective_repeat_original);
  //ACTIVATE(2, selective_repeat_original);
  */
  
  //This pre-built table setup piece would ideally be replaced with a connection request with handshake and so on.
  switch(ThisStation) {
    case 1:
      neighbours[0].stationID = 2;
      neighbours[1].stationID = 3;
      neighbours[2].stationID = -1;
      neighbours[3].stationID = -1;
      break;
    case 2:
      neighbours[0].stationID = 1;
      neighbours[1].stationID = 3;
      neighbours[2].stationID = -1;
      neighbours[3].stationID = -1;
      break;
    case 3:
      neighbours[0].stationID = 1;
      neighbours[1].stationID = 2;
      neighbours[2].stationID = -1;
      neighbours[3].stationID = -1;
      break;
  }
  
  ACTIVATE(1, FakeNetworkLayer_Test1);
  ACTIVATE(1, selective_repeat);
  
  ACTIVATE(2, FakeNetworkLayer_Test1);
  ACTIVATE(2, selective_repeat);
  
  ACTIVATE(3, FakeNetworkLayer_Test1);
  ACTIVATE(3, selective_repeat);
  
//asdasd
  /* simuleringen starter */
  Start();
  exit(0);
}


