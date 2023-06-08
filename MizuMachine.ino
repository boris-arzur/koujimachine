// $ arduino-cli core search avr
// $ arduino-cli core install arduino:avr
// $ arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 KoujiMachine.ino
// $ sudo usermod -a -G uucp $USER
// $ killall minicom
// $ arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=16MHzatmega328 KoujiMachine

#include <inttypes.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMPERATURE_PIN 4
OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensor(&oneWire);

#define LED(x) digitalWrite(13, x);

#define OPEN(x) digitalWrite(3, x)
#define CLOSE(x) digitalWrite(2, x)

#define CLOSED 0
#define OPENED 1
#define LOCKED 2

void idle()
{
  OPEN(0);
  CLOSE(0);
}

void set_faucet(int direction)
{
  if (direction == OPENED) { CLOSE(0); OPEN(1); }
  else                     { OPEN(0); CLOSE(1); }

  Serial.print((direction == OPENED)?"OPEN":"CLOSE");
  Serial.println();
  delay(2000 /* ms */);
  idle();
}

#define UPDATE_LED 10000 /* ms */
using seconds_t = unsigned long;

unsigned long start_ = 0;
int seconds_ = 0;
int next_time_ = 0;

int led_ = 0;
int next_action_ = OPENED;

void setup()
{
  Serial.begin(9600); // 9600 8N1
  Serial.println("online");
  delay(100);

  // test routine
  set_faucet(CLOSED);
  delay(1000);
  set_faucet(OPENED);
  delay(1000);
  set_faucet(CLOSED);

  next_time_ = 0 /* s */;
  next_action_ = OPENED;
  seconds_ = 1;
  start_ = millis();
}

void loop()
{
  static int need_flush = 0;
  struct X { ~X() { if (need_flush) { Serial.println(); } } } __x; // scope clean
  need_flush = 0;
#define SHOW(x)               \
    need_flush = 1; \
    Serial.print(" " #x ":"); \
    Serial.print(x);

  sensor.requestTemperatures();
  const auto temp = sensor.getTempCByIndex(0);

  // stop water when we dip below 22, restart when above 24.
  const auto do_lock = (temp < 22.0) || ((next_action_ == LOCKED) && temp < 24.0);

  SHOW(temp)
  SHOW(do_lock)

  if (do_lock) {
    if (next_action_ != LOCKED) { /* we just locked */ set_faucet(CLOSED); }
    next_action_ = LOCKED;
    SHOW(next_action_)
    delay(1000);
    return;
  }

  const auto now = millis();

  if (next_action_ == LOCKED) {
    // we just unlocked.
    next_action_ = OPENED;
    // dont blink the led, do open the faucet just now.
    next_time_ = 0;
    seconds_ = 1;
    start_ = now;
  }

  const auto delta = now - start_;
  if (delta > UPDATE_LED) {
    LED(led_);
    led_ = !led_;
    seconds_ += delta / 1000;
    start_ = now;

    SHOW(next_action_)
    SHOW(next_time_)
    SHOW(seconds_)
  }

  if (seconds_ >= next_time_) {
    seconds_ = 0;
    if (next_action_ == CLOSED) {
      set_faucet(CLOSED);
      next_action_ = OPENED;
      next_time_ = 3600 /* s */;
    } else {
      set_faucet(OPENED);
      next_action_ = CLOSED;
      next_time_ = 30 /* s */;
    }
  }

  delay(10001);
}
