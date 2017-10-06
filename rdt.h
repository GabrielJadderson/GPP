/*
 * rdt.h
 *
 *  Created on: Aug 18, 2017
 *      Author: jacob
 */

#ifndef RDT_H_
#define RDT_H_

/* Events */
#define network_layer_allowed_to_send  0x00000004
#define network_layer_ready            0x00000008
#define data_for_network_layer         0x00000010

#define frame_timer_timeout_millis  250
#define act_timer_timeout_millis     50

#define MAX_PKT 8        /* determines packet size in bytes */

typedef enum {false, true} boolean;        /* boolean type */
typedef unsigned int seq_nr;        /* sequence or ack numbers */
typedef struct {char data[MAX_PKT];} packet;        /* packet definition */
typedef enum {DATA, ACK, NAK} frame_kind;        /* frame_kind definition */


typedef struct {        /* frames are transported in this layer */
  frame_kind kind;        /* what kind of a frame is it? */
  seq_nr seq;           /* sequence number */
  seq_nr ack;           /* acknowledgement number */
  packet info;          /* the network layer packet */
  int sendTime;
  int recvTime;
} frame;

/* init_frame fills in default initial values in a frame. Protocols should
 * call this function before creating a new frame. Protocols may later update
 * some of these fields. This initialization is not strictly needed, but
 * makes the simulation trace look better, showing unused fields as zeros.
 */
void init_frame(frame *s, int count);

/* Fetch a packet from the network layer for transmission on the channel. */
void from_network_layer(packet *p);

/* Deliver information from an inbound frame to the network layer. */
void to_network_layer(packet *p);

/* Go get an inbound frame from the physical layer and copy it to r. */
int from_physical_layer(frame *r);

/* Pass the frame to the physical layer for transmission. */
void to_physical_layer(frame *s);

/* Start the clock running and enable the timeout event. */
void start_timer(seq_nr k);

/* Stop the clock and disable the timeout event. */
void stop_timer(seq_nr k);

/* Start an auxiliary timer and enable the ack_timeout event. */
void start_ack_timer(void);

/* Stop the auxiliary timer and disable the ack_timeout event. */
void stop_ack_timer(void);

/* Allow the network layer to cause a network_layer_ready event. */
void enable_network_layer(void);

/* Forbid the network layer from causing a network_layer_ready event. */
void disable_network_layer(void);

/* In case of a timeout event, it is possible to find out the sequence
 * number of the frame that timed out (this is the sequence number parameter
 * in the start_timer function). For this, the simulator must know the maximum
 * possible value that a sequence number may have. Function init_max_seqnr()
 * tells the simulator this value. This function must be called before
 * start_simulator() function. When a timeout event occurs, function
 * get_timedout_seqnr() returns the sequence number of the frame that timed out.
 */
void init_max_seqnr(unsigned int o);
unsigned int get_timedout_seqnr(void);

/* Macro inc is expanded in-line: Increment k circularly. */
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0

#endif /* RDT_H_ */
