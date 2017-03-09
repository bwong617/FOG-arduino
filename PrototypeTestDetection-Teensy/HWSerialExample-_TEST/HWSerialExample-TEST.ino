#define HWSERIAL Serial3

void setup() {
  Serial.begin(9600);
  HWSERIAL.begin(9600);
}

void loop() {   
  if (Serial.available() > 0) {
    HWSERIAL.write("AT+PIO21");
    delay(4000); //replace with the number of samples in this time period (as brandon was saying)
    HWSERIAL.write("AT+PIO20");
  }
  if (HWSERIAL.available() > 0) {
    //do nothing.. can take this if statement off.
  }
}

