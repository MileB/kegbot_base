#ifndef KEGBOT_BASE_H
#define KEGBOT_BASE_H

#include <stdio.h>
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
const int Flow_Meter1 = 23;
const int Flow_Meter2 = 24; 
const int M0          = 17;
const int M1          = 27;
const int M2          = 22;
const int RESET       = 5;
const int END 		  = 6;
const int NUM_METERS  = 16;

// Timespec is SECONDS, NANOSECONDS
const struct timespec SLEEP_TIME = {0, 10*1000};

// When to write to the database
// Ticks per gallon, in pints, per oz
const int RESET_CONDITION = (10300.0 / 8.0 / 16.0); 

//-----------------
//    OBJECTS
//----------------
// TODO: Set this as a hashed password
db_access db = db_access("kegbot", "root", "kegbot123", NUM_METERS);

//-----------------
//    FUNCTIONS
//----------------

/* Func: worker_thread
*  Inputs: none
*  Outputs: none
*  Worker thread to run for the life of the main program
*  Updates the active database in the background
*/
void worker_thread(void);

#endif
