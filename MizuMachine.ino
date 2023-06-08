// $ arduino-cli core search avr
// $ arduino-cli core install arduino:avr
// $ arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328
// KoujiMachine.ino $ sudo usermod -a -G uucp $USER $ killall minicom $
// arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=16MHzatmega328
// KoujiMachine

#include <DallasTemperature.h>
#include <OneWire.h>
#include <inttypes.h>

#define TEMPERATURE_PIN 4
OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensor(&oneWire);

#define ONE_LOOP 100 /* ms */

using celcius_t = float;
using seconds_t = double;
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
  const seconds_t elapsed = delta * 1.0e-3;
  seconds_ += elapsed;
  start = now;
}

void update_serial(celcius_t temperature, seconds_t back_to_open) {
#define SHOW(x) Serial.print(" " #x ":"); Serial.print(x);
    static seconds_t last = seconds_;
    if (seconds_ - last > 30) {
        SHOW(seconds_)
        SHOW(temperature)
        SHOW(back_to_open)
        Serial.println();
        last = seconds_;
    }
}

celcius_t update_temp() {
  sensor.requestTemperatures();
  return sensor.getTempCByIndex(0);
}

seconds_t update_cycle(celcius_t temp) {
    if (temp < 24) return 6 * 3600;
    if (temp > 34) return 1800;
    const auto x = (temp - 24.0) / (34.0 - 24.0);
    return x * (1800 - 6 * 3600) + 6 * 3600;
}

void update_faucet(seconds_t back_to_open) {
#define CLOSED 0
#define OPENED 1
  static int state = CLOSED;
  static seconds_t back_to_idle = 0;
  static seconds_t back_to_close = 0;

#define OPEN(x) digitalWrite(3, x)
#define CLOSE(x) digitalWrite(2, x)
  const auto idle = [&] { OPEN(0); CLOSE(0); back_to_idle = 0; };
  const auto open = [&] { OPEN(1); state = OPENED; back_to_idle = 2; back_to_close = 30; };
  const auto close = [&] { CLOSE(1); state = CLOSED; back_to_idle = 2; back_to_close = 0; };

  if (back_to_idle > 0 && seconds_ >= back_to_idle) { idle(); return; }
  if (back_to_close > 0 && seconds_ >= back_to_close) { close(); return; }
  if (state == CLOSED && seconds_ >= back_to_open) { open(); return; }
}

void setup() {
  Serial.begin(9600); // 9600 8N1
  Serial.println("online");

  update_faucet(0);
}

void loop() {
  const auto temperature = update_temp();
  const auto faucet_time = update_cycle(temperature);
  update_led(faucet_time);
  update_seconds();
  update_faucet(faucet_time);
  update_serial(temperature, faucet_time);

  delay(ONE_LOOP);
}
