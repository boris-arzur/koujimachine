// freakduino chibi v1.1B is atm328p at 8MHz
//   + arduino:avr:pro:cpu=8MHzatmega328

// fasttech ships a small footprint arduino board:
//   + arduino:avr:pro:cpu=8MHzatmega328
//   * 3 USD, shipping included
//   * https://www.fasttech.com/products/0/10013165/2215700-nano-3-0-atmel-atmega328p-mini-usb-board-for

// I use a FR ELECTRONICS ZRA6010A at pin 13,
// https://www.fasttech.com/products/0/10036087/7867702-mager-1-channel-3-32v-ssr-solid-state-relay-module probably works as well.

// $ ls KoujiMachine/KoujiMachine.ino
// ...
// $ arduino-cli core search avr
// $ arduino-cli core install arduino:avr
// $ arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 KoujiMachine.ino
// $ ls -l /dev/ttyUSB0
//crw-rw---- 1 root uucp 188, 0 Jan  8 14:49 /dev/ttyUSB0
// $ sudo usermod -a -G uucp $USER
// $ killall minicom
// $ arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=8MHzatmega328 KoujiMachine
// $ #echo 1:0:80000cbd:0:3:1c:7f:15:4:5:1:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0 | stty -F /dev/ttyUSB0; cat /dev/ttyUSB0
// $ minicom -D /dev/ttyUSB0 -b 9600

#define KOUJI 0
#define BRISKET 1

#if KOUJI
#define CONSIGNE 37.0
#define TOO_HIGH 40.0 /* strict no overshoot */
#define TIMEBASE 1000.0
#define UPDATE_PID (1.0 * TIMEBASE)
#define PID_CYCLE (20.0 * TIMEBASE)
#elif BRISKET
#define CONSIGNE 68.0 // 57 is low 68 is high
#define TOO_HIGH 70.0 /* strict no overshoot */
#define TIMEBASE (60 * 1000.0)
#define UPDATE_PID (1.0 * TIMEBASE)
#define PID_CYCLE (0.1 * TIMEBASE)
#endif

#define SPEED 0.16
#define KP (4 / (SPEED * PID_CYCLE))
#define KI (0.1 * KP)
#define KD (1000 * KP)
#define START 0.3

float last_temp;
float integrale;
float pid;

unsigned long update_cycle_start;
unsigned long pid_cycle_start;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600); // 9600 8N1 in minicom...
  Serial.println("online");
  analogReference(EXTERNAL); // we connect 3.3V on AREF

  delay(100);

  last_temp = readAnalogTM61(A0);
  integrale = START * 1 / KI;
  pid = START;
  update_cycle_start = millis();
  pid_cycle_start = millis();
}

#define AREF_STEP_TO_BIASED_TEMP 3.3 / 1024 * 100.0
#define TEMP_BIAS 0.6 * 100
float readAnalogTM61(int pin) {
  const int read_ = analogRead(pin);
  return AREF_STEP_TO_BIASED_TEMP * read_ - TEMP_BIAS;
}

void loop() {
  const unsigned long now = millis();
  const float tempA0 = readAnalogTM61(A0);
  const float tempA1 = readAnalogTM61(A1);

  if (now - update_cycle_start > UPDATE_PID) {
    const float temp = (tempA0 + tempA1) / 2;
    const float delta = CONSIGNE - temp; 
    const float ddelta = 1000.0 * (temp - last_temp) / (now - update_cycle_start);

    integrale = integrale + (now - update_cycle_start) / 1000.0 * delta;
    if (integrale < 0) integrale = 0;
    if (integrale > 1.0 / KI) integrale = 1 / KI;

    pid = KP * delta + KI * integrale - KD * ddelta;

    update_cycle_start = now;
    last_temp = temp; 

#define SHOW(x) \
    Serial.print(" " #x ":"); \
    Serial.print(x); \

    SHOW(tempA0)
    SHOW(tempA1)
    SHOW(temp)

    auto show = [](const char* r, float v, float p) {
      Serial.print(" ");
      Serial.print(r);
      Serial.print(":");
      Serial.print(v);
      Serial.print("[");
      Serial.print(p);
      Serial.print("]");
    };

    show("P", delta, KP * delta); 
    show("I", integrale, KI * integrale); 
    show("D", ddelta, - KD * ddelta); 

    SHOW(pid)
  }
 
  // FR ELECTRONICS ZRA6010A at pin 13,
  // brisket : 40 W lamp in water, in a pan, well insulated (electric & thermal)
  // kouji : 200 W hair drier insid cardboard box
  if (now - pid_cycle_start > PID_CYCLE) {
    digitalWrite(13, HIGH); // start cycle warming ON
    pid_cycle_start = now;
  } 
  
  if ((now - pid_cycle_start) > pid * PID_CYCLE ||
      tempA1 > TOO_HIGH ||
      tempA0 > TOO_HIGH) {
    digitalWrite(13, LOW); // turn warming OFF
  } 
}

/* how to plug things: text

USB up
 * left side
   + reset : nothing
   + 3.3v : right side AREF
   + 5v : white
   + GND : green
   + GND : nothing
   + vin : nothing

   + analog 0 : red
   + analog 1 : yellow
   + ... : nothing

 * right side
   + aref : right side 3.3v
   + gnd : gnd switch
   + output 13 : input switch

*/

/* how to plug things: webp
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
0pf9gL2v7hv04bMChFwAAAAAAAA=
*/
