/*
 * debug.h
 *
 *  Created on: Aug 20, 2017
 *      Author: jacob
 */

#ifndef DM557_PROJEKT_2017_DEBUG_H_
#define DM557_PROJEKT_2017_DEBUG_H_

typedef enum {trace, debug, info, warn, error, succes} log_level;

int logLine(log_level level, const char * __restrict format, ... );

#endif /* DM557_PROJEKT_2017_DEBUG_H_ */
