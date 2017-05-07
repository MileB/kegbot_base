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

static std::condition_variable cv;
static std::mutex cv_m;
static bool shutdown = false;

int main (){

  /* Set up SIGTERM interrupt handler */
  shutdown = false;
  if (signal(SIGTERM, sig_handler) == SIG_ERR)
    syslog(LOG_WARNING,"Cannot catch SIGTERM. Will not be able to" 
        "flush ticks before exit");

  if (!db.is_safe()) {
    syslog(LOG_ERR, "Database Access couldn't start!");
    return -1;
  }

  /* Spawn off helper threads */
  std::thread active_runner (active_thread);
  std::thread archive_runner(archive_thread);
  std::thread sensor_runner (sensor_thread);

  active_runner.join();
  archive_runner.join();
  sensor_runner.join();
  syslog(LOG_INFO, "Shutdown complete");
  return 0;
}

void sig_handler(int signo)
{
  if (signo == SIGTERM){
    syslog(LOG_INFO, "SIGTERM Caught, performing clean exit...");
    std::unique_lock<std::mutex> lock (cv_m);
    shutdown = true;
    cv.notify_all();
  }
}

void sensor_thread(void)
{
  /* Count will loop through
   * all possible control signals */
  uint8_t count = 0;
  std::unique_lock<std::mutex> lock (cv_m);

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

  /* Keep track of current a past pulses
   * to catch falling edge conditions */
  int Previous_Pulses [NUM_METERS] = {0};
  int Current_Pulse   [NUM_METERS] = {0};

  while (!shutdown) {

    /* Overflow, reset count */
    if (count > 7){
      count = 0;
    }

    /* Set correct control signal bits
     * based on count */
    digitalWrite(M0, count & 0x01);
    digitalWrite(M1, count & 0x02);
    digitalWrite(M2, count & 0x04);

    /* Sleep placed here to allow the multiplexer
     * signals to settle before polling */
    cv.wait_for(lock, std::chrono::nanoseconds(PIN_RATE_NS));

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
}

void archive_thread(void)
{
  std::unique_lock<std::mutex> lock (cv_m);
  const int sleep_ms = db.get_archive_rate();

  while (!shutdown) {
    cv.wait_for(lock, std::chrono::milliseconds(sleep_ms));
    syslog(LOG_INFO, "Updating Archive");
    db.archive();
  }
}

void active_thread(void)
{
  std::unique_lock<std::mutex> lock (cv_m);
  const int sleep_ms = db.get_active_rate();

  while (!shutdown) {
    cv.wait_for(lock, std::chrono::milliseconds(sleep_ms));
    db.update(false);
  }
}
