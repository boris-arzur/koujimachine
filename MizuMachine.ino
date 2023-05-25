// $ arduino-cli core search avr
// $ arduino-cli core install arduino:avr
// $ arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 KoujiMachine.ino
// $ sudo usermod -a -G uucp $USER
// $ killall minicom
// $ arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=16MHzatmega328 KoujiMachine

#define LED(x) digitalWrite(13, x);
#define OPEN(x) digitalWrite(2, x)

#define CLOSE(x) digitalWrite(3, x)
#define DO_ACTUATE(open) \
  if (open) { CLOSE(0); OPEN(1); } \
  else { OPEN(0); CLOSE(1); }


#define CLOSED 1
#define OPENED 0

void set_faucet(int direction)
{
  DO_ACTUATE(direction)
  delay(1000 /* ms */);
  OPEN(0);
  CLOSE(0);
}

#define UPDATE_LED 10000 /* ms */
unsigned long start_ = 0;
int led_ = 0;

int seconds_ = 0;
int next_time_ = 0;
int next_action_ = CLOSED;

void setup()
{
  set_faucet(CLOSED);
  next_action_ = CLOSED;
  next_time_ = 0;
  start_ = millis();
}

void loop()
{
  const auto now = millis();
  if (now - start_ > UPDATE_LED) {
    LED(led_);
    led_ = !led_;
    seconds_ += 10;
    start_ = now;
  }

  if (seconds_ > next_time_) {
    seconds_ = 0;
    if (next_action_ == CLOSED) {
      set_faucet(CLOSED);
      next_action_ = OPENED;
      next_time_ = 7200; // in 2 hours
    } else {
      set_faucet(OPENED);
      next_action_ = CLOSED;
      next_time_ = 60; // in 1 minute
    }
  }

  delay(100);
}
