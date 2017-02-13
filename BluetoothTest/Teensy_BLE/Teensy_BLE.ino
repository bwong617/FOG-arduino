#include <SoftwareSerial.h>
 
SoftwareSerial BTSerial(0, 1);
 
void setup() {
   Serial.begin(9600);
   BTSerial.begin(9600);
   //BTSerial.write("AT+DEFAULT\r\n");
   //BTSerial.write("AT+RESET\r\n");
   //BTSerial.write("AT+NAME=FastForward\r\n");
   //BTSerial.write("AT+ROLE1\r\n");
   //BTSerial.write("AT+TYPE1"); //Simple pairing
   BTSerial.write("AT+CONC6C4D0F72FCD");
}
 
void loop()
{
   if (BTSerial.available())
       Serial.write(BTSerial.read());
   if (Serial.available())
       BTSerial.write(Serial.read());
    //BTSerial.write("HELLO");

  /*char c;
  if (Serial.available()) {
    c = Serial.read();
    BTSerial.print(c);
  }
  if (BTSerial.available()) {
    c = BTSerial.read();
    Serial.print(c);    
  }*/
}
