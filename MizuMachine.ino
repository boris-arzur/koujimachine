// On a nano clone, with a l298n h-bridge controling a motorized faucet.
// $ arduino-cli core search avr
// $ arduino-cli core install arduino:avr
// $ arduino-cli compile --fqbn arduino:avr:pro:cpu=16MHzatmega328
// arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=16MHzatmega328
// MizuMachine

#include <DallasTemperature.h>
#include <OneWire.h>
#include <inttypes.h>
#include <math.h>

namespace {

#define TEMPERATURE_PIN 4
OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensor(&oneWire);

#define ONE_LOOP 100 /* ms */

using celcius_t = float;
using seconds_t = unsigned long;
using milliseconds_t = unsigned long;

void update_led(milliseconds_t led_speed) {
#define LED(x) digitalWrite(13, x);
  static int led_on = 0;
  static milliseconds_t start = millis();

  const auto now = millis();
  const auto delta = now - start;
  if (delta > led_speed) {
    LED(led_on);
    led_on = !led_on;
    start = now;
  }
}

seconds_t seconds_ = 0;
void update_seconds() {
  static milliseconds_t start = millis();

  const auto now = millis();
  const auto delta = now - start;
  const seconds_t elapsed = delta / 1000;
  const milliseconds_t left = delta - elapsed * 1000;
  seconds_ += elapsed;
  start = now - left;
}

using state_t = int;
#define Start 0
#define Closed 1
#define Opened 2
#define Closing 3
#define Opening 4
#define Idling 5

void print_state(state_t state) {
#define PRINT(x_)                                                              \
  case x_:                                                                     \
    Serial.print(#x_);                                                         \
    break;

  switch (state) {
    PRINT(Start)
    PRINT(Closed)
    PRINT(Opened)
    PRINT(Closing)
    PRINT(Opening)
    PRINT(Idling)
  default:
    Serial.print("INVALID");
    break;
  }
}

void update_serial(celcius_t temperature, seconds_t back_to_open, state_t state) {
#define SHOW(x)                                                                \
  Serial.print(" " #x ":");                                                    \
  Serial.print(x);

  static seconds_t last = seconds_;
  static int last_state = 0;
  if (last_state != state || seconds_ - last > 10) {
    print_state(state);
    SHOW(seconds_)
    SHOW(temperature)
    SHOW(back_to_open)
    Serial.println();
    last = seconds_;
    last_state = state;
  }
}

celcius_t update_temp() {
  sensor.requestTemperatures();
  static const celcius_t bias = 0.5; // a bit hotter at location
  return sensor.getTempCByIndex(0) - bias;
}

seconds_t update_cycle(celcius_t temp) {
  // settings
  static constexpr float asymptotic_cycle = 1800.0;

  static constexpr float temp_a = 30.0;
  static constexpr float cycle_a = 3600.0 * 6;

  static constexpr float temp_b = 34.0;
  static constexpr float cycle_b = 3600.0;

  // constants
  static constexpr float alpha = 1.0 / (temp_b - temp_a) * log((cycle_a - asymptotic_cycle)/ (cycle_b - asymptotic_cycle));
  static constexpr float kappa = (cycle_a - asymptotic_cycle) * exp(alpha * temp_a);

  // runtime
  return asymptotic_cycle + kappa * exp(-alpha * temp);
}

state_t update_faucet(seconds_t back_to_open) {
  static state_t state = Closed;
  static seconds_t back_to_idle = 0;
  static seconds_t back_to_close = 0;

#define OPEN(x) digitalWrite(3, x)
#define CLOSE(x) digitalWrite(2, x)

  const auto idle = [&] {
    OPEN(0);
    CLOSE(0);
    back_to_idle = 0;
    return Idling;
  };
  const auto open = [&] {
    OPEN(1);
    state = Opened;
    back_to_idle = 2;
    back_to_close = 30;
    seconds_ = 0;
    return Opening;
  };
  const auto close = [&] {
    CLOSE(1);
    state = Closed;
    back_to_idle = 2;
    back_to_close = 0;
    return Closing;
  };

  if (back_to_idle > 0 && seconds_ >= back_to_idle) {
    return idle();
  }
  if (back_to_close > 0 && seconds_ >= back_to_close) {
    return close();
  }
  if (state == Closed && seconds_ >= back_to_open) {
    return open();
  }
  return state;
}

} // anon namespace

void setup() {
  Serial.begin(9600); // 9600 8N1
  Serial.println("online");

  update_serial(0, 0, update_faucet(0));
}

void loop() {
  const auto temperature = update_temp();
  const auto faucet_time = update_cycle(temperature);
  update_led(faucet_time);
  update_seconds();
  const auto state = update_faucet(faucet_time);
  update_serial(temperature, faucet_time, state);

  delay(ONE_LOOP);
}
