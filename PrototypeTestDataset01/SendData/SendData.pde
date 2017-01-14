import processing.serial.*;
import java.io.*;

int counter = 0;
String [] subtext;
String outputData;

//initialize Serial object
Serial myPort;


void setup()
{
  size(200,200);
  
  // select serial port
  // println("Available serial ports:");
  // printArray(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 38400);
}


void draw()
{
  
  readData("/home/christina/Documents/FYDP/FOG-arduino/PrototypeTestDataset01/S01R01_freeze.txt");
  
  if (counter < subtext.length){
    //myPort.write(subtext[counter] + ",");
    print("\t00: " + subtext[counter]);
    counter++;
  }
  
  if (myPort.available() > 0) {
    String inBuffer = myPort.readStringUntil('\n');   
    if (inBuffer != null) {
      println(inBuffer);
    }
  }

}

//void serialEvent(Serial port) {
//  outputData = port.readString();
//  print(outputData);
//}

/* The following function will read from a CSV or TXT file */
void readData(String myFileName)
{
  File file=new File(myFileName);
  BufferedReader br=null;
  
  try{
    
    br = new BufferedReader(new FileReader(file));
    String text = null;
    
    /* keep reading each line until you get to the end of the file */
    while((text=br.readLine()) != null){
      /* Split each line up into bits and pieces using a comma as a separator */
      subtext = splitTokens(text,",");
    }
    
  }catch(FileNotFoundException e){
    e.printStackTrace();
  }catch(IOException e){
    e.printStackTrace();
  }finally{
    try {
      if (br != null){
        br.close();
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
}