/**************************
 * CMPE-450/CMPE-451 - KEGBOT
 * KEGBOT_BASE 
 *
 *  Michael Byers
 *  Kyle    Henry
 *  Sam     Sullivan
 *  Yue     Yu
 */

#include "kegbot_base.h"

/* Global boolean to manage CTRL+C 
 * or SIGINT exit condition */
static bool shutdown;

int main (){

  /* Set up CTRL+C sigint interrupt handler */
  shutdown = false;
  if (signal(SIGINT, sig_handler) == SIG_ERR)
    printf("WARNING! Cannot catch SIGINT. CTRL+C will not flush ticks before exit\n");

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

  std::thread active_runner (active_thread);
  std::thread archive_runner(archive_thread);

  /* Count will loop through
   * all possible control signals */
  uint8_t count = 0;
  int Previous_Pulses [NUM_METERS] = {0};
  int Current_Pulse   [NUM_METERS] = {0};


  while(!shutdown){

    /* Overflow, reset count */
    if (count > 7){
      count = 0;
    }

    /* Set correct control signal bits
     * based on count */
    digitalWrite(M0, count & 0x01);
    digitalWrite(M1, count & 0x02);
    digitalWrite(M2, count & 0x04);

    /* Sleep used to needlessly maxing out the CPU.
     * Sleep placed here to allow the multiplexer
     * signals to settle before polling */
    nanosleep(&PIN_SLEEP_TIME, NULL);

    /* Check / store last pulse results in previous pulse
     * Used to find edges of the signals */
    /* Read from Flow_Meter 1 */
    Previous_Pulses[count] = Current_Pulse[count];
    Current_Pulse[count] = digitalRead(Flow_Meter1);

    /* Read from Flow_Meter 2 */
    Previous_Pulses[count+8] = Current_Pulse[count+8];
    Current_Pulse[count+8] = digitalRead(Flow_Meter2);

    /* Detected a falling edge on first MUX, current count */
    if ((Current_Pulse[count] == 0)&&(Previous_Pulses[count] == 1)){
      db.add(count, 1); 
    }

    /* Detected a falling edge on second MUX, current count+8 */
    if ((Current_Pulse[count+8] == 0)&&(Previous_Pulses[count+8] == 1)){
      db.add(count+8, 1); 
    }

    count++;
  }

  return 0;
}

void sig_handler(int signo)
{
  if (signo == SIGINT)
  {
    printf("SIGINT Caught, flushing ticks\n");
    db.update(true);
    db.archive();
    printf("Finished flushing database, shutting down\n");
    shutdown = true;
  }
}

void archive_thread(void)
{
  struct timespec archive_sleep = db.get_archive_rate();
  while(!shutdown)
  {
    nanosleep(&archive_sleep, NULL);
    db.update(false);
  }
}

void active_thread(void)
{
  struct timespec active_sleep = db.get_active_rate();
  while(!shutdown)
  {
    nanosleep(&active_sleep, NULL);
    db.archive();
  }
}
