/*
 * debug.c
 *
 *  Created on: Aug 20, 2017
 *      Author: jacob
 */
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "subnetsupport.h"
#include "subnet.h"
#include "debug.h"

extern mlock_t *write_lock;
extern int ThisStation;



int logLine(log_level level, const char * __restrict format, ... ) {
    va_list arg;
    int rv;
    char *level_string;

	int log_level_limit = succes; // INFO: Here you can set the log level you wish to output

	switch(level) {
	case trace:
		level_string = "TRACE: ";
		break;
	case debug:
		level_string = "DEBUG: ";
		break;
	case info:
		level_string = "INFO:  ";
		break;
	case warn:
		level_string = "WARN:  ";
		break;
	case error:
		level_string = "ERROR: ";
		break;
	case succes:
		level_string = "SUCCES: ";
		break;
	}



    if( level >= log_level_limit) {
        Lock (write_lock );
        char buffer[255] = "";
        char buffer2[255] = "";

        va_start(arg, format);
        rv = vsnprintf( buffer, 255, format, arg);
        va_end(arg);
        sprintf(buffer2, "%s %s %s", GetProcessName(), level_string, buffer);
        SyncLog(buffer2, strlen(buffer2));
        printf("%d %s %s", ThisStation, level_string, buffer2);

        Unlock(write_lock);

    } else {
    	rv = 0;
    }

    return rv;
}


