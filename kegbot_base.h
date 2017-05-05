#ifndef KEGBOT_BASE_H
#define KEGBOT_BASE_H

#include <stdio.h>
#include <signal.h>
#include <wiringPi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <thread>
#include <time.h>
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

// Timespec is SECONDS, NANOSECONDS
const struct timespec SLEEP_TIME = {0, 10*1000};

// When to write to the database
// Ticks per gallon, in pints, per oz
const int RESET_CONDITION = (10300.0 / 8.0 / 16.0); 

// (1 / max frequency of low meter) / 2 / 8 (in seconds) * 1000000 (in nano-seconds)
const int PIN_RATE = (1.0/343.0/2.0/8.0 * 1000000.0);

const struct timespec PIN_SLEEP_TIME = {0, PIN_RATE};

//-----------------
//    OBJECTS
//----------------
db_access db = db_access("./kegbot.cfg");

//-----------------
//    FUNCTIONS
//----------------

/* TODO: Comment these */
void active_thread(void);
void archive_thread(void);

void sig_handler(int);

#endif
