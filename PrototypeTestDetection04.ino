//Max V: -0.3V to 3.6V -> use 3.3V input

#define LOG_OUT 1 // use the log output function
#define FFT_N 16 // set to 256 point fft
#include <FFT.h>  // include the FFT library

//const int xpin = A2;      // x-axis of the accelerometer
//const int ypin = A1;      // y-axis 
const int zpin = A0;      // z-axis
//const int sensorPin = 3;  // visual and tactile cues
int sampleDelay = 100;    // delay (ms) between readings

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

//FFT Variables (WIP)

//int fft_size;
//int sample_rate;
//int update_rate;
//int iteration;
//int max_iterations;
//int orientation;

//16 MHz / 128 = 125 KHz, inside the desired 50-200 KHz range.
//For each 'bin' N the frequency is N * Sample_Rate / FFT_Size
// FFT_Size = FFT_N = 256
//sample_Rate = 64; 200ms -> 5 samples/second
// Frequency = N * 115200 / 256 = N * 450

int FFT_Calc_Rate;

void setup() {
  
  // Initialize serial communication
  // Sampling Rate = max 115200Hz
  Serial.begin(4800);

  // Tell the analog-to-digital converter (ADC) to use an
  // external reference voltage (AREF pin)
  analogReference(EXTERNAL);

  // Accelerometer data input pins
  //pinMode(xpin, INPUT);
  //pinMode(ypin, INPUT);
  pinMode(zpin, INPUT);
  
  // Visual and tactile cues
  //pinMode(sensorPin, OUTPUT);
  
  //FFT Setup (WIP)
  //TIMSK0 = 0; // turn off timer0 for lower jitter
  //ADCSRA = 0xe5; // set the adc to free running mode
  //ADMUX = 0x40; // use adc0
  //DIDR0 = 0x01; // turn off the digital input for adc0

  //fft_size = 256;
  //sample_rate = 64;
  //update_rate = 2;
  //iteration = 0;
  //max_iterations = sample_rate/update_rate;
  //orientation = 1;
  
  FFT_Calc_Rate = 4;
}

void loop() {
  
  // ======================== ATTEMPT 2 ========================
  // Test Conditions:   - Baud Rate     = 4800
  //                    - FFT_N         = 16
  //                    - FFT_Calc_Rate = 4
  int z_data = 0;
  int z_acc = 0;
  int i = 0;
  int fft_count = 0;
  bool fft_init_flag = 0;
  Serial.print("***");
  
  while(true) { // reduces jitter

    z_data = analogRead(zpin);
    z_acc = (1000*((float)z_data - zero_G)/scale);
    
    fft_input[i] = z_acc;   // put real data into even bins
    fft_input[i+1] = 0;     // set odd bins to 0
    
    Serial.print(z_acc);
    Serial.print("\t");
    
    if (i >= FFT_N*2){
      fft_init_flag = true;
    }
    
    fft_count++;
    i+=2;
    
    if ((fft_count >= FFT_Calc_Rate) && (fft_init_flag == true)){
      fft_window();      // window the data for better frequency response
      fft_reorder();     // reorder the data before doing the fft
      fft_run();         // process the data in the fft
      fft_mag_log();     // take the output of the fft
      
      Serial.println("");
      Serial.print("FFT:\t\t\t\t\t\t");
      
      for (byte j = 0 ; j < FFT_N/2 ; j++) {
        Serial.print(fft_log_out[j]);
        Serial.print("\t");
      }
      
      Serial.println("");
      Serial.print("Z_ACC:\t");
      
      if (i >= FFT_N*2){
        i = 0;
      }
      fft_count = 0;
    }
    
  }
  
  /* ======================== ATTEMPT 1 ========================
  int z_data = 0;
  int z_acc = 0;
  
  while(1) { // reduces jitter
    Serial.println("");
    Serial.print("TIME:\t");
    Serial.print(millis());
    Serial.println("");
    Serial.print("Z_ACC:\t");
  
    for (int i = 0 ; i < FFT_N*2 ; i += 2) {   
      z_data = analogRead(zpin);
      z_acc = (1000*((float)z_data - zero_G)/scale);
      
      fft_input[i] = z_acc;   // put real data into even bins
      fft_input[i+1] = 0;     // set odd bins to 0
      
      Serial.print(z_acc);
      Serial.print("\t");
    }
    Serial.println("");
    Serial.print("TIME:\t");
    Serial.print(millis());
    Serial.println("");
    Serial.print("FFT:\t");
    
    fft_window();      // window the data for better frequency response
    fft_reorder();     // reorder the data before doing the fft
    fft_run();         // process the data in the fft
    fft_mag_log();     // take the output of the fft
    
    for (byte i = 0 ; i < FFT_N/2 ; i++) {
      Serial.print(fft_log_out[i]);
      Serial.print("\t\t");
    }
    //Serial.println("");
    //Serial.print("TIME:\t");
    //Serial.print(millis());
    //Serial.println("");
    //Serial.print("PSD:\t");
    //for (byte i = 0 ; i < FFT_N/2 ; i++) {
    //  Serial.print((int)abs(pow(fft_log_out[i],2)));
    //  Serial.print("\t\t");
    //}
  }
  
  */
  //delay(sampleDelay);
}

/*
- 256 Samples for FFT
- Calc FFT after every 32 samples
- Sampling rate 64Hz
*/
