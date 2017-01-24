//Max V: -0.3V to 3.6V -> use 3.3V input

//// Current Setup:
// - 256 Samples for FFT
// - Sampling rate 64Hz (64 samples per second)
// - Calc FFT after every 32 samples, after the first 256 are collected
// - Locomotor Band: 0.5Hz - 3.0Hz
// - Freeze Band: 3.0Hz - 8.0Hz
// - Freeze Threshold: 1      (Istvan used 3)
// - Energy Threshold: 4000    (Istvan used 4000)

#define ARM_MATH_CM4
#include <arm_math.h>

#define FFT_SIZE 256     // Set to number of samples for FFT *****PARAM*****

//// Arduino Pin Declarations
const int ACCL_Z_INPUT_PIN = 14;     // z-axis acceleration input pin
const int LED_PIN = 13;    // LED to indicate when data is read from the file
const int MOTOR_PIN = 5;  // Tactile cue output pin
const int LASER_PIN = 6;  // Visual cue output pin

//// Accelerometer Data Scaling

  // scale is the number of units we expect the sensor reading to change when the acceleration along an axis changes by 1G.
  //   Datasheet: 1G -> 330 mV/G
  //   ADC (0-1023): map 0V to 0ADC, 3.3V to 1023ADC
  //   0G -> 512ADC
  //   0.330 V/G × (1023 ADC units) / 3.3 V = 102.3 (ADC units)/G
double scale = 102.3;

  //   zero_G is the reading we expect from the sensor when it detects
  //   no acceleration. Subtract this value from the sensor reading to
  //   get a shifted sensor reading.
double zero_G = 512.0;

//// Sampling
const int SAMPLE_RATE_HZ = 64;                                    // 64Hz = 64 samples per second *****PARAM*****
const unsigned long SAMPLE_INTERVAL = 1000000 / SAMPLE_RATE_HZ;    // Interval of time between each sample collection in microseconds
float BIN_SIZE = (float)SAMPLE_RATE_HZ/(float)FFT_SIZE;              // Bin size of FFT output data (Hz)

//// Timers
unsigned long fog_millis = 0;
bool end_freeze_flag = false;
bool end_cue_flag = false;

//// Input data variables and counters
int i;                                     // Input data index
int z_data;                                // Voltage data retrieved from analogRead
double z_acc;                              // Input data converted to acceleration in milliG
const int fft_calc_rate = 32;              // Number of new samples collected before FFT is recalculated *****PARAM*****

//// Threshold and Cutoff Inputs *****PARAM*****
const float freeze_threshold = 1.0;            // *May vary person-to-person 
const int energy_threshold = 4000;
float locomotor_lower_cutoff = 0.5;        // Locomotor Band: 0.5Hz - 3.0Hz
float locomotor_upper_cutoff = 3;
float freeze_lower_cutoff = 3;             // Freeze Band: 3.0Hz - 8.0Hz
float freeze_upper_cutoff = 8;

//// Power Integration Outputs
float integration_multiplier;              // Used in Trapezoid Rule discrete integration
float locomotor_band;                      // Magnitude sums of power bands
float freeze_band;
float freeze_index;                        // Compare indices with thresholds to identify FOG; If both indices exceed their corresponding thresholds, FOG is identified
float energy_index;

//****DATA SET****
//// Variables for dataset version
String inString = "";
//****DATA SET****

////////////////////////////////////////////////////////////////////////////////
// INTERNAL STATE
////////////////////////////////////////////////////////////////////////////////

IntervalTimer samplingTimer;
float samples[FFT_SIZE*2];
float magnitudes[FFT_SIZE];
int sampleCounter = 0;

////////////////////////////////////////////////////////////////////////////////
// MAIN SKETCH FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial communication
  // Max Teensy Baud Rate = 38400Hz
  Serial.begin(38400);
  
  // Tell the analog-to-digital converter (ADC) to use an external reference voltage (AREF pin)
  analogReference(EXTERNAL);

  // Accelerometer data input pin
  pinMode(ACCL_Z_INPUT_PIN, INPUT);
  
  // Cue control output pin
  pinMode(LED_PIN, OUTPUT);
  /*pinMode(motor_pin, OUTPUT);
  pinMode(laser_pin, OUTPUT);*/
  
  // Initialize variables
  i = 0;
  z_data = 0;
  z_acc = 0;;
  integration_multiplier = 0;
  locomotor_band = 0;
  freeze_band = 0;
  freeze_index = 0;
  energy_index = 0;

  // Scale the cutoff frequencies based on the number of samples (FFT_N) / sampling rate
  locomotor_lower_cutoff *= (1/BIN_SIZE);
  locomotor_upper_cutoff *= (1/BIN_SIZE);
  freeze_lower_cutoff *= (1/BIN_SIZE);
  freeze_upper_cutoff *= (1/BIN_SIZE);

  // Begin sampling acceleration
  ////samplingBegin();
  samplingTimer.begin(samplingCallback, SAMPLE_INTERVAL);
}

void loop() {
    // Calculate FFT if a full sample is available.
    if (samplingIsDone()) {
      Serial.println("FFT");
      // Run FFT on sample data.
      arm_cfft_radix4_instance_f32 fft_inst;
      arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
      arm_cfft_radix4_f32(&fft_inst, samples);
      // Calculate magnitude of complex numbers output by the FFT.
      arm_cmplx_mag_f32(samples, magnitudes, FFT_SIZE);
  
      processValues();
      cueControl();
  
      // Restart sampling.
      samplingBegin();
    }
}

////////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void processValues() {
  //// Integrate to get the magnitude sum over each band (Trapezoid Rule: http://tutorial.math.lamar.edu/Classes/CalcII/ApproximatingDefIntegrals.aspx )
  // Note: Bin size: SAMPLE_RATE_HZ/FFT_SIZE = 64/256 = 0.25 -> 0, 0.25, 0.50, 0.75, 1.00, ...
  for (int k = 0; k <= freeze_upper_cutoff; k++){
    // Characteristic of the Trapezoid Rule method for discrete integration
    integration_multiplier = 1 / (float)SAMPLE_RATE_HZ;
          
    // If bin k is the first or last bin of either band
    if(k == locomotor_lower_cutoff || k == locomotor_upper_cutoff || k == freeze_lower_cutoff || k == freeze_upper_cutoff){
      integration_multiplier /= 2;
    }
    // If bin k is within the locomotor band range, add the magnitude of the FFT output squared
    if(k >= locomotor_lower_cutoff && k <= locomotor_upper_cutoff){
      locomotor_band += magnitudes[k] * integration_multiplier;
    }
    // If bin k is within the freeze band range, add the magnitude of the FFT output squared
    if(k >= freeze_lower_cutoff && k <= freeze_upper_cutoff){
      freeze_band += magnitudes[k] * integration_multiplier;
    }
  }
      
  // Calculate Freeze Index and Energy Index
  freeze_index = freeze_band / locomotor_band;
  energy_index = freeze_band + locomotor_band;
      
  // Output the results to the serial monitor for reference
  Serial.print("\tLB: "); Serial.print(locomotor_band);
  Serial.print("\tFB: "); Serial.print(freeze_band);
  Serial.print("\tFI: "); Serial.print(freeze_index);
  Serial.print("\tEI: "); Serial.print(energy_index);
}

void cueControl() {
        // Cue Control: If both thresholds are exceeded, FOG is identified -> trigger the cues
      if (freeze_index > freeze_threshold && energy_index > (float)energy_threshold){
        // Trigger cue pin HIGH
        ////digitalWrite(motor_pin, HIGH);
        ////digitalWrite(laser_pin, HIGH);
        Serial.println("\t\tFOG");
        digitalWrite(LED_PIN, HIGH);
        end_freeze_flag = true;
      }
      else{                // If FOG is not occurring, or no longer occurring, turn off cues
        // Trigger cue pin LOW
        /*if (end_freeze_flag == true){
          end_freeze_flag = false;
          end_cue_flag = true;
          fog_millis = 0;
          Serial.println("\t\tEND");
        }
        else{
          Serial.println("\t\t...");
        }
        //digitalWrite(motor_pin, LOW);
        //digitalWrite(laser_pin, LOW);
        //Serial.println("\t\t...");*/
        digitalWrite(LED_PIN, LOW);
      }
      
}

////////////////////////////////////////////////////////////////////////////////
// SAMPLING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void samplingCallback() {
  // Read from the accelerometer and store the sample data
  samples[sampleCounter] = (float32_t)analogRead((ACCL_Z_INPUT_PIN - zero_G)/scale);
  //****DATA SET****
  /*if (Serial.available() > 0) {  
    // *READ FROM DATASET FILE (SERIAL.READ)
    inString = "";
    int inChar;
  
    do {
      inChar = Serial.read();
      if (isDigit(inChar)){
        inString += (char)inChar;
      }
    } while ((char)inChar != ',');     
    samples[sampleCounter] = inString.toFloat();
    // Complex FFT functions require a coefficient for the imaginary part of the input.
    // Since we only have real data, set this coefficient to zero.
    samples[sampleCounter+1] = 0.0;
    // Update sample buffer position and stop after the buffer is filled
    sampleCounter += 2;
    if (sampleCounter >= FFT_SIZE*2) {
      samplingTimer.end();
    }    
        
  } */
  //****DATA SET****

  // Complex FFT functions require a coefficient for the imaginary part of the input.
  // Since we only have real data, set this coefficient to zero.
  samples[sampleCounter+1] = 0.0;
  // Update sample buffer position and stop after the buffer is filled
  sampleCounter += 2;
  if (sampleCounter >= FFT_SIZE*2) {
    samplingTimer.end();
  }
}

void samplingBegin() {
  // Reset sample buffer position and start callback at necessary rate.
  //sampleCounter = 0;
  // Shift carry-over input data; note that "0" data points do not need to be carried over
  //   (To maintain the order of sampled data, newer sample data is shifted to override older sample data at the beginning
  //    of the FFT input list, to make room at the end of the list for new sample data to be recorded)
  for (int j = fft_calc_rate*2 ; j < FFT_SIZE*2 ; j += 2) {   
        samples[j - (fft_calc_rate * 2)] = samples[j];
  }

  // Reset the sample index such that new samples do not replace the carry-over input data (sample order is maintained)
  sampleCounter = (FFT_SIZE - fft_calc_rate) * 2;
      
  // Reset the variables that collect a sum of powers
  locomotor_band = 0;
  freeze_band = 0;
  samplingTimer.begin(samplingCallback, SAMPLE_INTERVAL);
}

boolean samplingIsDone() {
  return sampleCounter >= FFT_SIZE*2;
}

