#ifndef DB_TEST_H
#define DB_TEST_H

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <syslog.h>

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <thread>
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

// (1 / max frequency of low meter) / 2 / 8 (in seconds) * 1000000 (in nano-seconds)
const int PIN_RATE_NS = (1.0/343.0/2.0/8.0 * 1000000000.0);

//-----------------
//    OBJECTS
//----------------
static db_access db("/etc/kegbot.cfg");

//-----------------
//    FUNCTIONS
//----------------

/* TODO: Comment these */
void active_thread(void);
void archive_thread(void);
void sensor_thread(void);

void sig_handler(int);

#endif
