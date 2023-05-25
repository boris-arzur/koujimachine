// $ arduino-cli core search avr
// $ arduino-cli core install arduino:avr
// $ arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 KoujiMachine.ino
// $ sudo usermod -a -G uucp $USER
// $ killall minicom
// $ arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=16MHzatmega328 KoujiMachine

#include <inttypes.h>

#define LED(x) digitalWrite(13, x);

#define OPEN(x) digitalWrite(2, x)
#define CLOSE(x) digitalWrite(3, x)

#define CLOSED 0
#define OPENED 1

void idle()
{
  OPEN(0);
  CLOSE(0);
}

void set_faucet(int direction)
{
  if (direction == OPENED) { CLOSE(0); OPEN(1); }
  else                     { OPEN(0); CLOSE(1); }

  delay(2000 /* ms */);
  idle();
}

#define UPDATE_LED 10000 /* ms */
// 2
//using seconds_t = int; // 21 B
using seconds_t = unsigned long; // 25 B ok.

//2 * sizeof(int) + x = 21
//2 * sizeof(unsigned long) + x = 25
// alignof?
//template<int size_>
//struct S { };
//S<sizeof(int)>::wut;
// -> sizeof(int) = 2
//S<sizeof(unsigned long)>::wut;
// -> sizeof(unsigned long) = 2

unsigned long start_ = 0;
int seconds_ = 0;
int next_time_ = 0;

int led_ = 0;
int next_action_ = OPENED;

void setup()
{
  set_faucet(CLOSED);
  next_action_ = OPENED;
  next_time_ = 20 /* s */;
  seconds_ = 0;
  start_ = millis();
}

void loop()
{
  const auto now = millis();
  const auto delta = now - start_;

  if (delta > UPDATE_LED) {
    LED(led_);
    led_ = !led_;
    seconds_ += delta / 1000;
    start_ = now;
  }

  if (seconds_ > next_time_) {
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

  delay(100);
}
