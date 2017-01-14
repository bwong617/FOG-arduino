//Max V: -0.3V to 3.6V -> use 3.3V input

//// Current Setup:
// - 256 Samples for FFT
// - Sampling rate 64Hz (64 samples per second)
// - Calc FFT after every 32 samples, after the first 256 are collected
// - Locomotor Band: 0.5Hz - 3.0Hz
// - Freeze Band: 3.0Hz - 8.0Hz
// - Freeze Threshold: 2      (Istvan used 3)
// - Energy Threshold: 500    (Istvan used 4000 ~ 2^12)

//// FFT Library Defines ( http://wiki.openmusiclabs.com/wiki/Defines )
#define LOG_OUT 1     // Use the log output function
#define FFT_N 256     // Set to number of samples for FFT *****PARAM*****
#include <FFT.h>      // include the FFT library

//// Arduino Pin Declarations
const int z_pin = A0;     // z-axis acceleration input pin
const int LED_pin = 4;    // LED to indicate when data is read from the file
const int motor_pin = 5;  // Tactile cue output pin
const int laser_pin = 6;  // Visual cue output pin

//// Accelerometer Data Scaling

  // scale is the number of units we expect the sensor reading to change when the acceleration along an axis changes by 1G.
  //   Datasheet: 1G -> 330 mV/G
  //   ADC (0-1023): map 0V to 0ADC, 3.3V to 1023ADC
  //   0G -> 512ADC
  //   0.330 V/G × (1023 ADC units) / 3.3 V = 102.3 (ADC units)/G
double scale = 102.3;

  // zero_G is the reading we expect from the sensor when it detects
  //   no acceleration. Subtract this value from the sensor reading to
  //   get a shifted sensor reading.
double zero_G = 512.0;

//// Sampling
const int sampling_rate = 64;                                    // 64Hz = 64 samples per second *****PARAM*****
const unsigned long sampling_interval = 1000 / sampling_rate;    // Interval of time between each sample collection
float bin_size = (float)sampling_rate/(float)FFT_N;              // Bin size of FFT output data (Hz)

//// Timers
unsigned long curr_millis = 0;             // Timers used to set acceleration data sampling rate
unsigned long prev_millis = 0;

unsigned long fog_millis = 0;
bool end_freeze_flag = false;
bool end_cue_flag = false;

//// Input data variables and counters
int i;                                     // Input data index
int z_data;                                // Voltage data retrieved from analogRead
double z_acc;                              // Input data converted to acceleration in milliG
int z_acc_int;                             // z_acc converted from float to int (decimal places truncated)
const int fft_calc_rate = 32;              // Number of new samples collected before FFT is recalculated *****PARAM*****

//// Threshold and Cutoff Inputs *****PARAM*****
const float freeze_threshold = 1.0;            // *May vary person-to-person 
const int energy_threshold = 300;
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

//// Variables for dataset version
String inString = "";
int intData;

//=====================================================================================================================================================================//

void setup() {
  // Initialize serial communication
  // Max Arduino Baud Rate = 115200Hz
  Serial.begin(115200);
  
  // Tell the analog-to-digital converter (ADC) to use an external reference voltage (AREF pin)
  analogReference(EXTERNAL);

  // Accelerometer data input pin
  pinMode(z_pin, INPUT);
  
  // Cue control output pin
  pinMode(LED_pin, OUTPUT);
  pinMode(motor_pin, OUTPUT);
  pinMode(laser_pin, OUTPUT);
  
  // Initialize variables
  i = 0;
  z_data = 0;
  z_acc = 0;
  z_acc_int = 0;
  integration_multiplier = 0;
  locomotor_band = 0;
  freeze_band = 0;
  freeze_index = 0;
  energy_index = 0;

  // Scale the cutoff frequencies based on the number of samples (FFT_N) / sampling rate
  locomotor_lower_cutoff *= (1/bin_size);
  locomotor_upper_cutoff *= (1/bin_size);
  freeze_lower_cutoff *= (1/bin_size);
  freeze_upper_cutoff *= (1/bin_size);
}

//=====================================================================================================================================================================//

void loop() {
  
  Serial.println("Initial sampling, please wait...");
  
  while(1) { // Reduces jitter (according to the Internet)
    
    //---------- DATA COLLECTION ----------//
    
    // Update timer
    curr_millis = millis();
    
    // Collect a sample when the timer reaches the sampling interval
    if (curr_millis - prev_millis >= sampling_interval){
      
      // Read voltage data from accelerometer
      //////z_data = analogRead(z_pin);
      // Convert and scale the data to G's; multiply by 1000 to preserve decimal information prior to double->int conversion
      //////z_acc = 1000*((((double)z_data - zero_G)/scale)-1.0); //TO-DO: Empirical Calibration to properly account for zero-G (current solution is "-1.0")
      // Type cast double to int
      //////z_acc_int = z_acc;
      
      if (Serial.available() > 0) {
        
        // *READ FROM DATASET FILE (SERIAL.READ)
        int inChar;
        
        do {
          inChar = Serial.read();
          if (isDigit(inChar)){
            inString += (char)inChar;
          }
        } while ((char)inChar != ',');
        
        intData = inString.toInt();
        inString = "";
        digitalWrite(LED_pin, HIGH);
        
      } else {
        digitalWrite(LED_pin, LOW);
      }
      
      // Copy real data into even bins
      //////fft_input[i] = z_acc_int;
      fft_input[i] = intData;
      // Set odd bins to 0 (no imaginary input data)
      fft_input[i+1] = 0;
      
      // Reset timer for next sampling interval
      prev_millis += sampling_interval;
      // Increment sample index by two bins
      i+=2;
    }
    
    // 
    if (end_cue_flag == true && fog_millis == 0){
      fog_millis = curr_millis;
    }
    else if (end_cue_flag == true && (curr_millis - fog_millis) >= 10){ //2000
      digitalWrite(motor_pin, LOW);
      digitalWrite(laser_pin, LOW);
      end_cue_flag = false;
    }
    
    // When enough samples are collected to perform an FFT
    if (i >= FFT_N*2){
      
      //---------- FFT PROCESSING & POWER INTEGRATION ----------//
      
      // FFT library functions ( http://wiki.openmusiclabs.com/wiki/FFTFunctions )
      fft_window();      // Window the data for better frequency response
      fft_reorder();     // Reorder the data before doing the fft
      fft_run();         // Process the data in the fft
      fft_mag_log();     // Take the magnitude output of the fft
      
      //// Integrate to get the magnitude sum over each band (Trapezoid Rule: http://tutorial.math.lamar.edu/Classes/CalcII/ApproximatingDefIntegrals.aspx )
      // Note: Bin size: sampling_rate/FFT_N = 64/256 = 0.25 -> 0, 0.25, 0.50, 0.75, 1.00, ...
      for (int k = 0; k <= freeze_upper_cutoff; k++){
        // Characteristic of the Trapezoid Rule method for discrete integration
        integration_multiplier = 1 / (float)sampling_rate;
        
        // If bin k is the first or last bin of either band
        if(k == locomotor_lower_cutoff || k == locomotor_upper_cutoff || k == freeze_lower_cutoff || k == freeze_upper_cutoff){
          integration_multiplier /= 2;
        }
        // If bin k is within the locomotor band range, add the magnitude of the FFT output squared
        if(k >= locomotor_lower_cutoff && k <= locomotor_upper_cutoff){
          locomotor_band += pow(fft_log_out[k],2) * integration_multiplier;
        }
        // If bin k is within the freeze band range, add the magnitude of the FFT output squared
        if(k >= freeze_lower_cutoff && k <= freeze_upper_cutoff){
          freeze_band += pow(fft_log_out[k],2) * integration_multiplier;
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
      
      //---------- CUE CONTROL ----------//
      
      // Cue Control: If both thresholds are exceeded, FOG is identified -> trigger the cues
      if (freeze_index > freeze_threshold && energy_index > (float)energy_threshold){
        // Trigger cue pin HIGH
        digitalWrite(motor_pin, HIGH);
        digitalWrite(laser_pin, HIGH);
        Serial.println("\t\tFOG");
        end_freeze_flag = true;
      }
      else{                // If FOG is not occurring, or no longer occurring, turn off cues
        // Trigger cue pin LOW
        if (end_freeze_flag == true){
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
        //Serial.println("\t\t...");
      }
      
      //---------- PREPARATION FOR NEXT SAMPLING ----------//
      
      // Shift carry-over input data; note that "0" data points do not need to be carried over
      //   (To maintain the order of sampled data, newer sample data is shifted to override older sample data at the beginning
      //    of the FFT input list, to make room at the end of the list for new sample data to be recorded)
      for (int j = fft_calc_rate*2 ; j < FFT_N*2 ; j += 2) {   
        fft_input[j - (fft_calc_rate * 2)] = fft_input[j];
      }
      
      // Reset the sample index such that new samples do not replace the carry-over input data (sample order is maintained)
      i = (FFT_N - fft_calc_rate) * 2;
      
      // Reset the variables that collect a sum of powers
      locomotor_band = 0;
      freeze_band = 0;
        
    }//end if
       
  }//end while (never occurs)
  
}//end loop
