/*
* Reliable data transfer between two stations
*
* Author: Jacob Aae Mikkelsen.
* co-authors: Patrick Jakobsen(pajak16), Gabriel Jadderson(gajad16)
*/
int lalalala = 0;
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "events.h"
#include "networkLayer.h"
#include "rdt.h"
#include "subnetsupport.h"
#include "subnet.h"
#include "fifoqueue.h"
#include "debug.h"

#define NUM_MAX_NEIGHBOURS 4

//#include "transportLayer.c" //[PJ] Doing gross testing stuff because we're using a fake one this time around.
#include "networkLayer.c"

/* En macro for at lette overførslen af korrekt navn til Activate */
#define ACTIVATE(n, f) Activate(n, f, #f)

#define MAX_SEQ 127        /* should be 2^n - 1 */
#define NR_BUFS 4


/* Globale variable */

char *StationName;         /* Globalvariabel til at overføre programnavn      */
int ThisStation;           /* Globalvariabel der identificerer denne station. */
log_type LogStyle;         /* Hvilken slags log skal systemet føre            */
boolean network_layer_enabled;

//[PJ] This should be moved back when no longer messing with a fake transport layer.
#include "transportLayer.c"

LogBuf mylog;                /* logbufferen                                     */

//FifoQueue from_network_layer_queue;           		/* Queue for data from network layer */
//FifoQueue for_network_layer_queue;    /* Queue for data for the network layer */

//mlock_t *network_layer_lock;
//mlock_t *write_lock;
//extern mlock_t *write_lock;

neighbour neighbours[NUM_MAX_NEIGHBOURS]; //this array is global and contains all our neighbours, see rdt.h for neighbours struct.
extern networkAddress thisNetworkAddress;
extern NL_RoutingTable routingTable;

static boolean between(seq_nr a, seq_nr b, seq_nr c) //ensures that seq_nr b is within a and b
{
	boolean x = (a <= b) && (b < c);
	boolean y = (c < a) && (a <= b);
	boolean z = (b < c) && (c < a);
	logLine(debug, "a==%d, b=%d, c=%d, x=%d, y=%d, z=%d\n", a, b, c, x, y, z);

	return x || y || z;
}

/* Copies package content to buffer, ensuring it has a string end character. */
/*void packet_to_string(packet* data, char* buffer) //[PJ] As packets no longer exist in this fashion it doesn't have sense to have this function.
{
	strncpy(buffer, (char*)data->data, MAX_PKT);
	buffer[MAX_PKT] = '\0';

}*/

static void send_frame(frame_kind fk, seq_nr frame_nr, seq_nr frame_expected, datagram buffer[], neighbourid recipient)
{
	logLine(trace, "SF: send_frame: frame_nr=%d\n", frame_nr);
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
		neighbours[recipient].no_nak = false;        /* one nak per frame, please */
	}
	//logLine(succes, "Sending frame to physical layer for recipient: %d\n", recipient);
	to_physical_layer(&s, recipient);        /* transmit the frame */
	if (fk == DATA)
	{
		start_timer(recipient, frame_nr); //TODO
	}
	stop_ack_timer(0);        /* no need for separate ack frame */
}

//#include "rdt_fakeNetworkLayers.c" //[PJ] These are no longer valid. Kept around as reference material.

void log_event_received(long int event)
{
	char *event_name;
	switch (event) {
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

void selective_repeat()
{
	frame r; // scratch variable
        datagram *d; //scratch variable
        NL_OfferElement *o;

	neighbourid currentNeighbour = 0; //Each time this is used, it should be set.
	neighbour_SR_Data neighbourData[NUM_MAX_NEIGHBOURS];
	event_t event;
	long int events_we_handle;
	unsigned int timer_id;
        datagram *transfer_pointer; //Passing to the network layer with this prevents overwriting the buffer contents before the network layer has handled it.
        
        ConcurrentFifoQueue *offer;
        FifoQueue sendingQueue = InitializeFQ();
        FifoQueueEntry e;
        
        /*datagram ERRORDATAGRAM;
        ERRORDATAGRAM.type = DATAGRAM;
        ERRORDATAGRAM.reserved = 0;
        ERRORDATAGRAM.payloadsize = MAX_PAYLOAD;
        ERRORDATAGRAM.src = 666;
        ERRORDATAGRAM.dest = thisNetworkAddress;
        ERRORDATAGRAM.payload.data[0] = 'E';
        ERRORDATAGRAM.payload.data[1] = 'R';
        ERRORDATAGRAM.payload.data[2] = 'R';
        ERRORDATAGRAM.payload.data[3] = 'O';
        ERRORDATAGRAM.payload.data[4] = 'R';
        ERRORDATAGRAM.payload.data[5] = '!';
        ERRORDATAGRAM.payload.data[6] = '\n';
        ERRORDATAGRAM.payload.data[7] = '\0';
        
        datagram INITIALDATAGRAM;
        INITIALDATAGRAM.type = DATAGRAM;
        INITIALDATAGRAM.reserved = 0;
        INITIALDATAGRAM.payloadsize = MAX_PAYLOAD;
        INITIALDATAGRAM.src = 666;
        INITIALDATAGRAM.dest = thisNetworkAddress;
        INITIALDATAGRAM.payload.data[0] = 'E';
        INITIALDATAGRAM.payload.data[1] = 'R';
        INITIALDATAGRAM.payload.data[2] = 'R';
        INITIALDATAGRAM.payload.data[3] = 'O';
        INITIALDATAGRAM.payload.data[4] = 'R';
        INITIALDATAGRAM.payload.data[5] = '!';
        INITIALDATAGRAM.payload.data[6] = '\n';
        INITIALDATAGRAM.payload.data[7] = '\0';*/

	//write_lock = malloc(sizeof(mlock_t));
	//network_layer_lock = (mlock_t *)malloc(sizeof(mlock_t));

	//Init_lock(write_lock);
	//Init_lock(network_layer_lock);

	enable_network_layer();  // initialize

	logLine(trace, "Starting selective repeat %d\n", ThisStation);

	// Set all the timers to be not set.
	for (int i = 0; i < NUM_MAX_NEIGHBOURS; i++) {
		neighbours[i].ack_timer_id = -1;
                neighbours[i].no_nak = false;

		for (int j = 0; j < NR_BUFS; j++) {
			neighbours[i].timer_ids[j] = -1;
		}

		init_neighbour_SR_Data(&neighbourData[i]);
                //neighbourData[currentNeighbour].in_buf[i] = INITIALDATAGRAM;
                //enable_network_layer(i, NR_BUFS);
	}

	events_we_handle = frame_arrival | timeout | network_layer_ready | LL_SendingQueueOffer;

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
                logLine(info, "LL: Waiting for signals.\n");
		Wait(&event, events_we_handle);
		log_event_received(event.type);

		switch (event.type) {
		case network_layer_ready: // accept, save, and transmit a new frame
			logLine(succes /*trace*/, "Network layer delivers frame - lets send it\n");

			//currentNeighbour = event.msg;
                        //logLine(succes, "Sending delivered frame: next_frame_to_send=%d\n", neighbourData[currentNeighbour].next_frame_to_send);

			neighbourData[currentNeighbour].nbuffered = neighbourData[currentNeighbour].nbuffered + 1; // expand the window
			from_network_layer(&neighbourData[currentNeighbour].out_buf[neighbourData[currentNeighbour].next_frame_to_send % NR_BUFS], &currentNeighbour, &event); //fetch new packet
			send_frame(DATA, neighbourData[currentNeighbour].next_frame_to_send, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour); // transmit the frame
			inc(neighbourData[currentNeighbour].next_frame_to_send); // advance upper window edge
                        logLine(succes, "LL: Window after sending: ack_exp=%d, nbuf=%d, nxt_f_t_s=%d\n", neighbourData[currentNeighbour].ack_expected, neighbourData[currentNeighbour].nbuffered, neighbourData[currentNeighbour].next_frame_to_send);
			break;

		case frame_arrival: // a data or control frame has arrived
			currentNeighbour = from_physical_layer(&r);  // fetch incoming frame from physical layer
                        
                        //logLine(succes, "LL: Received a frame of kind: r.kind=%d\n", r.kind);
			if (r.kind == DATA) {
                                logLine(debug, "Received frame context: seq=%d, expected=%d, too_far=%d, no_nak=%d\n", r.seq, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].too_far, neighbours[currentNeighbour].no_nak);
				// An undamaged frame has arrived.
				if ((r.seq != neighbourData[currentNeighbour].frame_expected) && neighbours[currentNeighbour].no_nak) {
					send_frame(NAK, 0, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);
				}
				else {
					start_ack_timer(currentNeighbour);
				}
				if (between(neighbourData[currentNeighbour].frame_expected, r.seq, neighbourData[currentNeighbour].too_far) && (neighbourData[currentNeighbour].arrived[r.seq%NR_BUFS] == false)) {
					// Frames may be accepted in any order.
					neighbourData[currentNeighbour].arrived[r.seq % NR_BUFS] = true; // mark buffer as full
					neighbourData[currentNeighbour].in_buf[r.seq % NR_BUFS] = r.info; //insert data into buffer
                                        //logLine(succes, "LL: cN=%d; r: kind=%d, seq=%d, ack=%d; r.info: type=%d, reserved=%d, payloadsize=%d; r.info.payload.data=%s \n", currentNeighbour, r.kind, r.seq, r.ack, r.info.type, r.info.reserved, r.info.payloadsize, r.info.payload.data);
                                        
					while (neighbourData[currentNeighbour].arrived[neighbourData[currentNeighbour].frame_expected % NR_BUFS]) {
                                                //logLine(succes, "LL: going through arrived frames. currently arrived frame: %d\n", neighbourData[currentNeighbour].arrived[neighbourData[currentNeighbour].frame_expected % NR_BUFS]);
						// Pass frames and advance window.
                                                transfer_pointer = malloc(sizeof(datagram));
                                                *(transfer_pointer) = neighbourData[currentNeighbour].in_buf[neighbourData[currentNeighbour].frame_expected % NR_BUFS];
                                                to_network_layer(transfer_pointer);
						//to_network_layer(&neighbourData[currentNeighbour].in_buf[neighbourData[currentNeighbour].frame_expected % NR_BUFS]);
						neighbours[currentNeighbour].no_nak = true;
						neighbourData[currentNeighbour].arrived[neighbourData[currentNeighbour].frame_expected % NR_BUFS] = false;
                                                
                                                //neighbourData[currentNeighbour].in_buf[r.seq % NR_BUFS] = ERRORDATAGRAM;
                                                
						inc(neighbourData[currentNeighbour].frame_expected); // advance lower edge of receiver's window
						inc(neighbourData[currentNeighbour].too_far); // advance upper edge of receiver's window
						start_ack_timer(currentNeighbour); // to see if (a separate ack is needed
					}
				}
			}
			if ((r.kind == NAK) && between(neighbourData[currentNeighbour].ack_expected, (r.ack + 1) % (MAX_SEQ + 1), neighbourData[currentNeighbour].next_frame_to_send)) {
				send_frame(DATA, (r.ack + 1) % (MAX_SEQ + 1), neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);
			}

			logLine(info, "Are we between so we can advance window? ack_expected=%d, r.ack=%d, next_frame_to_send=%d\n", neighbourData[currentNeighbour].ack_expected, r.ack, neighbourData[currentNeighbour].next_frame_to_send);
                        //if (r.kind != DATA) {
			while (between(neighbourData[currentNeighbour].ack_expected, r.ack, neighbourData[currentNeighbour].next_frame_to_send)) {
				logLine(debug, "Advancing window %d\n", neighbourData[currentNeighbour].ack_expected);
				neighbourData[currentNeighbour].nbuffered = neighbourData[currentNeighbour].nbuffered - 1; //handle piggybacked ack

				stop_timer(currentNeighbour, neighbourData[currentNeighbour].ack_expected % NR_BUFS); //frame arrived intact
				inc(neighbourData[currentNeighbour].ack_expected);  //advance lower edge of sender's window
                                //logLine(succes, "LL: Window after receiving: ack_exp=%d, nbuf=%d, nxt_f_t_s=%d\n", neighbourData[currentNeighbour].ack_expected, neighbourData[currentNeighbour].nbuffered, neighbourData[currentNeighbour].next_frame_to_send);
			}
                        //}
                        //logLine(succes, "LL: New values: cN=%d, frame_expected=%d, ack_expected=%d\n", currentNeighbour, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].ack_expected);
			break;

		case timeout: // Ack timeout or regular timeout
		  // Check if it is the ack_timer
			timer_id = event.timer_id;
			//logLine(succes /*trace*/, "Timeout with id: %d - acktimer_id is %d for neighbour %d\n", timer_id, neighbours[currentNeighbour].ack_timer_id, currentNeighbour);

			//Figure out which neighbour to resend to.
			currentNeighbour = ((packetTimerMessage*)event.msg)->neighbour;

			if (timer_id == neighbours[currentNeighbour].ack_timer_id) { // Ack timer timer out
				logLine(debug, "This was an ack-timer timeout. Sending explicit ack.\n");

				free(event.msg);
				neighbours[currentNeighbour].ack_timer_id = -1; // It is no longer running
				send_frame(ACK, 0, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);        /* ack timer expired; send ack */
			}
			else {
				int timed_out_seq_nr = ((packetTimerMessage *)event.msg)->k;

				//logLine(succes /*debug*/, "Timeout for frame - need to resend frame %d\n", timed_out_seq_nr);
                                //logLine(succes, "  Timeout between result: ack_exp=%d, ^, nxt_f_t_s=%d, res=%d\n", neighbourData[currentNeighbour].ack_expected, neighbourData[currentNeighbour].next_frame_to_send, between(neighbourData[currentNeighbour].ack_expected, timed_out_seq_nr, neighbourData[currentNeighbour].next_frame_to_send));
                                if(between(neighbourData[currentNeighbour].ack_expected, timed_out_seq_nr, neighbourData[currentNeighbour].next_frame_to_send)) { //Redundant check that shouldn't be necessary, but it is. The stopping of the timers appearently doesn't work, causing resends with wrong sequence numbers.
				  send_frame(DATA, timed_out_seq_nr, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour);
                                }
			}
			break;
                      case LL_SendingQueueOffer:
                        logLine(info, "LL: Offered a queue by NL\n");
                        offer = (ConcurrentFifoQueue*) event.msg;
                        
                        offer->used = true;
                        
                        logLine(trace, "LL: Is offered queue empty?: %d\n", EmptyFQ(offer->queue));
                        
                        while(EmptyFQ(offer->queue) == 0) { //Transfer all elements to own queue.
                          logLine(trace, "LL: Handling queue element.\n");
                          e = DequeueFQ(offer->queue);
                          
                          o = malloc(sizeof(NL_OfferElement));
                          //*o = *((NL_OfferElement*) ValueOfFQE(e));
                          //d = malloc(sizeof(datagram));
                          //*d = *(ValueOfFQE(e));
                          
                          o->otherHostNeighbourid = ((NL_OfferElement*) ValueOfFQE(e))->otherHostNeighbourid;
                          o->dat = ((NL_OfferElement*) ValueOfFQE(e))->dat;
                          
                          logLine(info, "LL: o contains: n=%d\n", o->otherHostNeighbourid);
                          
                          EnqueueFQ(NewFQE((void*) o), sendingQueue);
                          logLine(trace, "LL: emptyness of sendingQueue: %d\n", EmptyFQ(sendingQueue));
                        }
                        
                        offer->used = false;
                        //Unlock(offer->lock);
                        
                        break;
		}
                
                for(int i = 0; i < NUM_MAX_NEIGHBOURS; i++) {
                  logLine(debug, "LL: bufslots=%d, empty=%d, entrydest=%d\n",
                    NR_BUFS-neighbourData[i].nbuffered > 0, EmptyFQ(sendingQueue));
                  if(NR_BUFS-neighbourData[i].nbuffered > 0
                      && EmptyFQ(sendingQueue) == 0
                      && ((NL_OfferElement*)ValueOfFQE(FirstEntryFQ(sendingQueue)))->otherHostNeighbourid == i
                    ) {
                    logLine(info, "LL: Queueing a packet in the outgoing buffer for neighbour %d, with oHNid=%d.\n", i, ((NL_OfferElement*)ValueOfFQE(FirstEntryFQ(sendingQueue)))->otherHostNeighbourid);
                    e = DequeueFQ(sendingQueue);
                    
                    currentNeighbour = i;
                    
                    neighbourData[currentNeighbour].nbuffered = neighbourData[currentNeighbour].nbuffered + 1; // expand the window
                    neighbourData[currentNeighbour].out_buf[neighbourData[currentNeighbour].next_frame_to_send % NR_BUFS] = ((NL_OfferElement*)ValueOfFQE(e))->dat;
                    send_frame(DATA, neighbourData[currentNeighbour].next_frame_to_send, neighbourData[currentNeighbour].frame_expected, neighbourData[currentNeighbour].out_buf, currentNeighbour); // transmit the frame
                    inc(neighbourData[currentNeighbour].next_frame_to_send); // advance upper window edge
                  }
                }
                
                if(EmptyFQ(sendingQueue)) {
                  ClearEvent(network_layer_allowed_to_send);
                  enable_network_layer();
                }

		/*logLine(succes, "LL: !!! Checking if there is buffer room: nbuf=%d\n", neighbourData[currentNeighbour].nbuffered);
                ClearEvent(network_layer_allowed_to_send); //We only want one at a time. Don't stack requests we can't handle anyway.
                if (neighbourData[currentNeighbour].nbuffered < NR_BUFS) {
                        logLine(succes, "There was buffer room.\n");
                        //ClearEvent(network_layer_allowed_to_send); //We only want one at a time. Don't stack requests we can't handle anyway.
                        enable_network_layer(currentNeighbour, NR_BUFS-neighbourData[currentNeighbour].nbuffered);
		}*/
		/*else {
			disable_network_layer();
		}*/
                //logLine(succes, "!!!!! %d\n", neighbourData[currentNeighbour].nbuffered);
	}
}

void init_neighbour_SR_Data(neighbour_SR_Data *ND)
{
	ND->ack_expected = 0;
	ND->next_frame_to_send = 0;
	ND->frame_expected = 0;
	ND->too_far = NR_BUFS;
	ND->nbuffered = 0;
	for (int i = 0; i < NR_BUFS; i++) {
		ND->arrived[i] = false;
	}
}

neighbourid stationID2neighbourindex(int stationID)
{
	int i = 0;
	while (neighbours[i].stationID != stationID) {
		i++;

		if (i >= NUM_MAX_NEIGHBOURS) {
			logLine(error, "stationID2neighboutindex: unable to find neighbour for station: %d\n", stationID);
			Stop();
		}
	}

	return i;
}

void enable_network_layer()
{
	//Lock(network_layer_lock);
	logLine(trace, "enabling network layer\n");
	//network_layer_enabled = true;
        
        //NL_RequestFromLL *req = malloc(sizeof(NL_RequestFromLL));
        //neighbourid *N = malloc(sizeof(neighbourid));
        //*N = n;
        //req->neighbour=n;
        //req->bufferSlotsAvailable = bufroom;
	Signal(network_layer_allowed_to_send, NULL);
	//Unlock(network_layer_lock);
}

/*void disable_network_layer(void)
{
	//Lock(network_layer_lock);
	logLine(trace, "disabling network layer\n");
	network_layer_enabled = false;
	//Unlock(network_layer_lock);
}*/

void from_network_layer(datagram *p, neighbourid *n,  event_t* e)
{
	/*FifoQueueEntry e;

	//Lock(network_layer_lock);
	e = DequeueFQ(from_network_layer_queue);
	//Unlock(network_layer_lock);

	if (!e) {
		logLine(error, "ERROR: We did not receive anything from the queue, like we should have\n");
	}
	else {
		memcpy(p, (char *)ValueOfFQE(e), sizeof(packet));
		free((void *)ValueOfFQE(e));
		DeleteFQE(e);
	}*/
  
  //Extract values from message.
  NL_OfferElement o = *((NL_OfferElement*) (e->msg));
  
  (*p) = o.dat;
  (*n) = o.otherHostNeighbourid;
  
  free(e->msg);
}


void to_network_layer(datagram *p)
{
  /*char * buffer;
  //Lock(network_layer_lock);
  
  buffer = (char *)malloc(sizeof(char) * (1 + MAX_PKT));
  packet_to_string(p, buffer);
  
  EnqueueFQ(NewFQE((void *)buffer), for_network_layer_queue);
  
  //Unlock(network_layer_lock);
  
  Signal(data_for_network_layer, NULL);*/
  
  // Just send the packet to the network layer directly. Semantically identical to what was done before with the "send one element at a time and use a QUEUE TO DO IT???" method.
  void *ptr = (void*) p;
  Signal(data_for_network_layer, ptr);
}


void print_frame(frame* s, char *direction)
{
	//char temp[MAX_PKT + 1];

	switch (s->kind) {
	case ACK:
		logLine(info, "%s: ACK frame. Ack seq_nr=%d\n", direction, s->ack);
		break;
	case NAK:
		logLine(info, "%s: NAK frame. Nak seq_nr=%d\n", direction, s->ack);
		break;
	case DATA:
		//packet_to_string(&(s->info), temp); //[PJ] Can no longer do this.
		//logLine(info, "%s: DATA frame [seq=%d, ack=%d, kind=%d, (%s)] \n", direction, s->seq, s->ack, s->kind, temp);
		logLine(info, "%s: DATA frame [seq=%d, ack=%d, kind=%d, %s] \n", direction, s->seq, s->ack, s->kind, s->info.payload.data);
		break;
	}
}

int from_physical_layer(frame *r)
{
	r->seq = 0;
	r->kind = DATA;
	r->ack = 0;

	int source, dest, length;

	logLine(trace, "Receiving from subnet in station %d\n", ThisStation);
	FromSubnet(&source, &dest, (char *)r, &length);
	print_frame(r, "received");
        logLine(info, "LL: Frame is received from neighbour %d, which is station %d.\n", stationID2neighbourindex(source), source);

	return stationID2neighbourindex(source);
}


void to_physical_layer(frame *s, neighbourid neighbour)
{
	print_frame(s, "sending");
        logLine(info, "LL: Frame is sent to neighbour %d, which is station %d.\n", neighbour, neighbours[neighbour].stationID);
	ToSubnet(ThisStation, neighbours[neighbour].stationID, (char *)s, sizeof(frame));
}


void start_timer(neighbourid neighbour, seq_nr k)
{
	packetTimerMessage *msg = malloc(sizeof(packetTimerMessage));
	msg->k = k;
	msg->neighbour = neighbour;

	neighbours[neighbour].timer_ids[k % NR_BUFS] = SetTimer(frame_timer_timeout_millis, (void *)msg);
	logLine(trace, "start_timer for neighbour=%d seq_nr=%d timer_ids=[%d, %d, %d, %d]\n", neighbour, k, neighbours[neighbour].timer_ids[0], neighbours[neighbour].timer_ids[1], neighbours[neighbour].timer_ids[2], neighbours[neighbour].timer_ids[3]);
}


void stop_timer(neighbourid neighbour, seq_nr k)
{
	int timer_id;
	char *msg;

	timer_id = neighbours[neighbour].timer_ids[k];
	logLine(trace, "stop_timer for seq_nr %d med id=%d\n", k, timer_id);

	if (StopTimer(timer_id, (void *)&msg)) {
		logLine(trace, "timer %d stoppet. msg: %s \n", timer_id, msg);
		free(msg);
	}
	else {
		logLine(trace, "timer %d kunne ikke stoppes. Måske er den timet ud?timer_ids=[%d, %d, %d, %d] \n", timer_id, neighbours[neighbour].timer_ids[0], neighbours[neighbour].timer_ids[1], neighbours[neighbour].timer_ids[2], neighbours[neighbour].timer_ids[3]);
	}
}


void start_ack_timer(neighbourid neighbour)
{
	if (neighbours[neighbour].ack_timer_id == -1)
	{
		logLine(trace, "Starting ack-timer\n");
		packetTimerMessage *msg = malloc(sizeof(packetTimerMessage));
		msg->k = -1;
		msg->neighbour = neighbour;
		neighbours[neighbour].ack_timer_id = SetTimer(act_timer_timeout_millis, (void *)msg);
		logLine(debug, "Ack-timer startet med id %d\n", neighbours[neighbour].ack_timer_id);
	}
}


void stop_ack_timer(neighbourid neighbour)
{
	char *msg;

	logLine(trace, "stop_ack_timer\n");
	if (StopTimer(neighbours[neighbour].ack_timer_id, (void*)&msg)) {
		logLine(trace, "timer %d stoppet. msg: %s \n", neighbours[neighbour].ack_timer_id, msg);
		free(msg);
	}
	neighbours[neighbour].ack_timer_id = -1;
}


extern int global_log_level_limit;

void initialize_linkLayer(int stationID);
int main(int argc, char *argv[])
{
	StationName = argv[0];
	ThisStation = atoi(argv[1]);

	if (argc == 3) {
		printf("Station %d: arg2 = %s\n", ThisStation, argv[2]);
		global_log_level_limit = atoi(argv[2]);
	}
	else {
		global_log_level_limit = succes;
	}

	mylog = InitializeLB("mytest");

	LogStyle = synchronized;
        
        initialize_debug();

	printf("Starting network simulation\n");

	//This pre-built table setup piece would ideally be replaced with a connection request with handshake and so on.
	/*switch (ThisStation) {
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
	}*/

	/*ACTIVATE(1, FakeNetworkLayer_Test1);
	ACTIVATE(1, selective_repeat);

	ACTIVATE(2, FakeNetworkLayer_Test1);
	ACTIVATE(2, selective_repeat);

	ACTIVATE(3, FakeNetworkLayer_Test1);
	ACTIVATE(3, selective_repeat);*/
        
        //Shared
        //printf("\nInitializing link layer?\n\n");
        initialize_linkLayer(ThisStation);
        //printf("\nInitializing network layer?\n\n");
        initialize_networkLayer(ThisStation);
        
        //Host A
        //printf("\nActivating station 1?\n\n");
        ACTIVATE(1, selective_repeat);
        ACTIVATE(1, networkLayerHost);
        ACTIVATE(1, fake_transportLayer);
        
        //Host B
        //printf("\nActivating station 2?\n\n");
        ACTIVATE(2, selective_repeat);
        ACTIVATE(2, networkLayerHost);
        ACTIVATE(2, fake_transportLayer);
        
        //Router 1
        //printf("\nActivating station 3?\n\n");
        ACTIVATE(3, selective_repeat);
        ACTIVATE(3, networkLayerRouter);
        
	/* simuleringen starter */
        //printf("\nStarting simulation?\n\n");
	Start();
	exit(0);
}

void initialize_linkLayer(int stationID) {
  switch(stationID) {
    case 1: //Host A
      neighbours[0].stationID = 3;
      neighbours[1].stationID = -1;
      neighbours[2].stationID = -1;
      neighbours[3].stationID = -1;
      break;
    case 2: //Host B
      neighbours[0].stationID = 3;
      neighbours[1].stationID = -1;
      neighbours[2].stationID = -1;
      neighbours[3].stationID = -1;
      break;
    case 3: //Router 1
      neighbours[0].stationID = 1;
      neighbours[1].stationID = 2;
      neighbours[2].stationID = -1;
      neighbours[3].stationID = -1;
      break;
    case 4: //Router 2
      
      //break;
    default:
      logLine(error, "LL: Link layer initialized for station without a case (%d) - Sending Stop Signal.\n", stationID);
      Stop();
  }
}


