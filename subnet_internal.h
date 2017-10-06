/* 	$Id: subnet_internal.h,v 1.11 1999/09/02 22:09:44 dim Exp $
 *
 * $RelId: DM40, subnet. Version 1.3 (#1) Sep 06 18:28:52 CEST 1999 $
 *
 * BESKRIVELSE: Header-fil. Indeholder prototyper til de funktioner der 
 * benyttes internt af subnetpakken.
 *
 * Steffen Schmidt.
 */

#ifndef _SUBNET_INTERNAL_H_
#define _SUBNET_INTERNAL_H_

#ifndef _SUBNETSUPPORT_INC_
/*
 * prototyper:
 *
 * Disse seks startes som threads; alle bruger-funktionerne startes via
 * "shell", de øvrige er systemets threads.
 */
void *shell(void *arg);

void *receiver(void *arg);
void *control(void *arg);
void *errortimer(void *arg);
void *timer(void *arg);
void *flow(void *arg);

/*
 * prototyper:
 *
 * "almindelige" funktioner.
 */
void signalhandler(int sig);
error_type transmit_error(void);
void close_station(int caller);

void transfer_frame(BufStr buffer);
void delay_frame(BufStr buffer, int mul_limit);
void re_delay_frame(PriQueueEntry e);

void PrintBuf(void *buf);
void PrintThreadName(void);

#endif /* _SUBNETSUPPORT_INC_ */

void *memcopy(void *dest, const void *src, size_t n);
ActiveTimeoutControl lookupTimeoutId(unsigned int id);
void removeTimeout(ActiveTimeoutControl act);

#endif /* _SUBNET_INTERNAL_H_ */
