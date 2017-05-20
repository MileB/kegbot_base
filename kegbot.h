#ifndef KEGBOT_BASE_H
#define KEGBOT_BASE_H

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <syslog.h>
#include <signal.h>
#include <thread>
#include <wiringPi.h>
#include "db_access.h"

//-----------------
//    CONSTANTS
//----------------
// BCM GPIO 
const int Flow_Meter1 = 19;
const int Flow_Meter2 = 25; 
const int M0          = 22;
const int M1          = 23;
const int M2          = 24;
const int RESET       = 5;
const int END 		    = 6;
const int NUM_METERS  = 16;
/* TODO: PIN_RATE HAS BEEN CHANGED. NEEDS TO BE TESTED. WASN'T NS BEFORE */
// (1 / max frequency of low meter) / 2 / 8 (in seconds) * 1000000 (in nano-seconds)
const int PIN_RATE_NS = (1.0/343.0/2.0/8.0 * 1000000000.0);

//-----------------
//    OBJECTS
//----------------
static db_access db("/etc/kegbot.cfg");

//-----------------
//    FUNCTIONS
//----------------

/* Func: sig_handler(int)
 * Handles the SIGTERM interrupt
 * Used to cleanly exit the program */
void sig_handler(int);

/* Func: active_thread(void)
 * worker thread used to update the 
 * active table in the database at
 * regular intervals */
void active_thread(void);

/* Func: archive_thread(void)
 * worker thread used to update the 
 * archive table in the database at
 * regular intervals */
void archive_thread(void);

/* Func: sensor_thread(void)
 * worker thread used to check the 
 * sensors for new values and to 
 * accumulate any new found ticks */
void sensor_thread(void);


#endif
