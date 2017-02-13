//#define SHOW_FREE_MEMORY

#ifdef SHOW_FREE_MEMORY
#include <MemoryFree.h>
#endif

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>

// define pins (varies per shield/board)
#define BLE_REQ   6
#define BLE_RDY   7
#define BLE_RST   4

#define LED	13

// create peripheral instance, see pinouts above
BLEPeripheral                    blePeripheral       = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);

// create service
BLEService                       testService         = BLEService("0000ffe0-0000-1000-8000-00805f9b34fb");
// create counter characteristic
BLEUnsignedShortCharacteristic   testCharacteristic  = BLEUnsignedShortCharacteristic("0000ffe1-0000-1000-8000-00805f9b34fb", BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify | BLEIndicate);
// create user description descriptor for characteristic
//BLEDescriptor                    testDescriptor      = BLEDescriptor("2901", "counter");

// last counter update time
unsigned long long               lastSent            = 0;
unsigned char chrValue = 0xFF;

void setup() {
	
//led debug

  pinMode(LED,OUTPUT);

  Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
  //Wait until the serial port is available (useful only for the Leonardo)
  //As the Leonardo board is not reseted every time you open the Serial Monitor
  //while(!Serial) {}
  delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

  blePeripheral.setLocalName("FFReceiver");
#if 1
  blePeripheral.setAdvertisedServiceUuid(testService.uuid());
#else
  const char manufacturerData[4] = {0x12, 0x34, 0x56, 0x78};
  blePeripheral.setManufacturerData(manufacturerData, sizeof(manufacturerData));
#endif

  // set device name and appearance
  blePeripheral.setDeviceName("FFReceiver");
  blePeripheral.setAppearance(0x0080);

  // add service, characteristic, and decriptor to peripheral
  blePeripheral.addAttribute(testService);
  blePeripheral.addAttribute(testCharacteristic);
  //blePeripheral.addAttribute(testDescriptor);

  // assign event handlers for connected, disconnected to peripheral
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  testCharacteristic.setEventHandler(BLEWritten, characteristicWritten);
  testCharacteristic.setEventHandler(BLESubscribed, characteristicSubscribed);
  testCharacteristic.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);

  // set initial value for characteristic
  testCharacteristic.setValue(0);

  // begin initialization
  blePeripheral.begin();

 if (Serial){
  	Serial.println(F("Hello from WsSaBLE"));
 }

#ifdef SHOW_FREE_MEMORY
  Serial.print(F("Free memory = "));
  Serial.println(freeMemory());
#endif

}

void loop() {
  BLECentral central = blePeripheral.central();

  if (central) {
    // central connected to peripheral
     if (Serial) {
		 Serial.print(F("Connected to central: "));
    	 Serial.println(central.address());
	 }
    // reset counter value
    testCharacteristic.setValue(0);

    while (central.connected()) {
      // central still connected to peripheral
      if (testCharacteristic.written()) {
        // central wrote new value to characteristic
        
        chrValue = testCharacteristic.value();
        Serial.println(chrValue);        
        switch(chrValue){
        	
        	case 0xAA:
        		
        		if (Serial) {
        			Serial.println(F("Buzz..."));
        		}
        		
        		for (int i=0; i<10;i++){
	        		digitalWrite(LED, HIGH);   // sets the LED on
					delay(50);                  // waits for a second
	  				digitalWrite(LED, LOW);    // sets the LED off
	  				delay(50);   	
        		}
          
        		testCharacteristic.setValue(0xFF);
        		
        		break;
       case 0xBB:
        		
        		if (Serial) {
        			Serial.println(F("Buzz..."));
        		}
        		
        		for (int i=0; i<10;i++){
	        		digitalWrite(LED, HIGH);   // sets the LED on
					delay(20);                  // waits for a second
	  				digitalWrite(LED, LOW);    // sets the LED off
	  				delay(20);   	
        		}
          
        		testCharacteristic.setValue(0xFF);
        		
        		break;
        		
        	case 0xCC:
        		// lowThreashold
        		if (Serial) {
        			Serial.println(F("Buzz..."));
        		}
        		
        		for (int i=0; i<5;i++){
	        		digitalWrite(LED, HIGH);   // sets the LED on
					delay(50);                  // waits for a second
	  				digitalWrite(LED, LOW);    // sets the LED off
	  				delay(50);   	
        		}
          
        		testCharacteristic.setValue(0xCC);
        		
        		break;
        		
        	case 0xDD:
        		// highThreashold
        		if (Serial) {
        			Serial.println(F("Buzz..."));
        		}
        		
        		for (int i=0; i<5;i++){
	        		digitalWrite(LED, HIGH);   // sets the LED on
					delay(50);                  // waits for a second
	  				digitalWrite(LED, LOW);    // sets the LED off
	  				delay(50);   	
        		}
          
          		delay(1000);
          		
        		testCharacteristic.setValue(0x57);
        		
        		break;
        	default:
        	
        		if (Serial) {
	        		Serial.println(F("UNRECOGNIZED VALUE"));
        		}
        		
	        	testCharacteristic.setValue(0xFF);
        		
        }
        
        lastSent = 0;
      }

    }

    // central disconnected
    if (Serial){
	    Serial.print(F("Disconnected from central: "));
	    Serial.println(central.address());
    }
    
  }
}

void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  if (Serial) {
	  Serial.print(F("Connected event, central: "));
	  Serial.println(central.address());
  }
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  if (Serial) {
	  Serial.print(F("Disconnected event, central: "));
	  Serial.println(central.address());
  }
}

void characteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // characteristic value written event handler
 // Serial.print(F("Characteristic event, writen: "));
 // Serial.println(testCharacteristic.value(), DEC);
  
  
}

void characteristicSubscribed(BLECentral& central, BLECharacteristic& characteristic) {
  // characteristic subscribed event handler
  if (Serial) {
  Serial.println(F("Characteristic event, subscribed"));
  }
}

void characteristicUnsubscribed(BLECentral& central, BLECharacteristic& characteristic) {
  // characteristic unsubscribed event handler
  if (Serial) {
  Serial.println(F("Characteristic event, unsubscribed"));
  }
}

