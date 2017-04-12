/**************************
 * CMPE-450/CMPE-451 - KEGBOT
 * MUX_TEST - Used for PoC 
 *  of testing multiplexers
 *  with 16 total inputs.
 *
 *  Michael Byers
 *  Kyle    Henry
 *  Sam     Sullivan
 *  Yue     Yu
 */

#include "kegbot_base.h"

int pulses[NUM_METERS]          = {0};
int Previous_Pulses[NUM_METERS] = {0};
int Current_Pulse[NUM_METERS]   = {0};

/* Count will loop through
 * all possible control signals */
uint8_t count;

int main (){
	wiringPiSetupGpio();

    /* Setup flow meters and internal pull-up */
	pinMode(Flow_Meter1, INPUT);
	pullUpDnControl(Flow_Meter1, PUD_UP);
	pinMode(Flow_Meter2, INPUT);
	pullUpDnControl(Flow_Meter2, PUD_UP);

	pinMode(RESET, INPUT);
	pullUpDnControl(RESET, PUD_UP);
	pinMode(END, INPUT);
	pullUpDnControl(END, PUD_UP);

    /* Set Control Pins as output */
	pinMode(M0, OUTPUT);
	pinMode(M1, OUTPUT);
	pinMode(M2, OUTPUT);

    /* Count will loop through
     * all possible control signals */
	count = 0;

    std::thread worker(worker_thread);

	while(true){

        /* Overflow, reset count */
		if (count > 7){
			count = 0;
		}

        /* Set correct control signal bits
         * based on count */
        digitalWrite(M0, count & 0x01);
        digitalWrite(M1, count & 0x02);
        digitalWrite(M2, count & 0x04);

        /* Check store last pulse results in previous pulse
         * Used to find edges of the signals */
		Previous_Pulses[count] = Current_Pulse[count];
		Current_Pulse[count] = digitalRead(Flow_Meter1);

		/* Read from Flow_Meter 2 */
		
		  Previous_Pulses[count+8] = Current_Pulse[count+8];
		  Current_Pulse[count+8] = digitalRead(Flow_Meter2);
		
		/* Detected a falling edge on first MUX, current count */
		if ((Current_Pulse[count] == 0)&&(Previous_Pulses[count] == 1)){
			pulses[count]++; 
		}

        /* Detected a falling edge on second MUX, current count+8 */
		if ((Current_Pulse[count+8] == 0)&&(Previous_Pulses[count+8] == 1)){
			pulses[count+8]++;
		}
        nanosleep(&PIN_SLEEP_TIME, NULL);
		count++;
	}
}

void worker_thread(void)
{
    uint8_t count=0;
    while(true)
    {
        /* Overflow, reset count */
		if (count > 7){
			count = 0;
		}
        /* Print out pulses from first Mutex's index */
        if (pulses[count] >= RESET_CONDITION){
            printf("Previous_Pulses[%d]: %d\n", count, pulses[count]);			
            db.add(count, pulses[count]);
            db.update();
            pulses[count] = 0;
        }
        /* Print out pulses from second Mutex's index */
        if (pulses[count+8] >= RESET_CONDITION){
            printf("Previous_Pulses[%d]: %d\n", count+8, pulses[count+8]);
            db.add(count+8, pulses[count+8]);
            db.update();
            pulses[count+8] = 0;
        }
        db.archive();
        nanosleep(db.sleep_time(), NULL);
        //nanosleep(&DB_SLEEP_TIME, NULL);
        count++;
    }
}
