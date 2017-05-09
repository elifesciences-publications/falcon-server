/*
 Send TTL pulses when data is received
 */

int ttlPin = 8 ;     // select the pin for the TTL pulses
int ttlDur = 100;    // pulse-width in microseconds
int BAUDRATE = 9600;  // bit per second

void setup() {
  pinMode(ttlPin, OUTPUT);    // declare the TTL pin as output
  Serial.begin(BAUDRATE);     // connect to the serial port at BAUDRATE bits per second
}

void loop () {
  if (Serial.read() > '0') {
      digitalWrite(ttlPin, HIGH);
      delayMicroseconds(ttlDur);
      digitalWrite(ttlPin, LOW);
  }
}
