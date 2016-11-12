//Max V: -0.3V to 3.6V -> use 3.3V input

const int xpin = A2;      // x-axis of the accelerometer
const int ypin = A1;      // y-axis 
const int zpin = A0;      // z-axis

const int sensorPin = 3;  // visual and tactile cues

int sampleDelay = 100;    // delay (ms) between readings

void setup() {
  
  // Initialize serial communication
  Serial.begin(9600);

  // Tell the analog-to-digital converter (ADC) to use an
  // external reference voltage (AREF pin)
  analogReference(EXTERNAL);

  // Accelerometer data input pins
  pinMode(xpin, INPUT);
  pinMode(ypin, INPUT);
  pinMode(zpin, INPUT);
  
  // Visual and tactile cues
  pinMode(sensorPin, OUTPUT);
  
}

void loop() {
  
  // Read data from accelerometer (ADC)
  int x_data = analogRead(xpin);
  int y_data = analogRead(ypin);
  int z_data = analogRead(zpin);
  
  // scale is the number of units we expect the sensor reading to
  // change when the acceleration along an axis changes by 1G.
  
  // Datasheet: 1G -> 330 mV/G
  // ADC (0-1023): map 0V to 0ADC, 3.3V to 1023ADC
  // 0G -> 512ADC
  // 0.330 V/G × (1023 ADC units) / 3.3 V = 102.3 (ADC units)/G
  float scale = 102.3;
  
  // zero_G is the reading we expect from the sensor when it detects
  // no acceleration. Subtract this value from the sensor reading to
  // get a shifted sensor reading.
  float zero_G = 512.0;
  
  // Divide the shifted sensor reading by scale to get acceleration in mGs.
  int x_acc = (1000*((float)x_data - zero_G)/scale);
  int y_acc = (1000*((float)y_data - zero_G)/scale);
  int z_acc = (1000*((float)z_data - zero_G)/scale);
  
  // Print values to serial monitor
  Serial.print(x_acc); Serial.print("\t");
  Serial.print(y_acc); Serial.print("\t");
  Serial.print(z_acc); Serial.println();
  
  // For Panel Exam demo purposes only (tilt the board to activate cues)
  if (x_acc > 500){
    digitalWrite(sensorPin, HIGH);
  }
  else{
    digitalWrite(sensorPin, LOW);
  }
  
  delay(sampleDelay);
}

//TODO: look into empirical calibration
//TODO: verify resolution
