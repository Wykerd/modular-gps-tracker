/*
 * Software fo the Snap|Click|Track SD module rev 1
 * Software version 1; Created 7 September 2018
 * 
 * This code is made for one way data transfer to SD card
 * 
 * Usage instructions to interface with other modules:
 * - Once the module is turnt on it waits for a string of data on sent to SerialSoftware(3,2)
 * - This string is loaded into the filename buffer. All data sent after this point will be written to the filename path.
 * - The software only supports writng of data, no data can be retrieved. Thie source code can be modified to send back data though serial.
 * - Send 64 bytes or less at a time!
 */

#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
SoftwareSerial ss = SoftwareSerial(3,2);

File dataFile;

int i = 0;

char filename[25];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //ss.begin(9600);
  Serial.println(F("BOOTING"));
  if (!SD.begin()) {
    Serial.println(F("SD FAILED"));
    return;
  }
  Serial.println(F("DONE"));
  while (!Serial.available()); //Do nothing
  while (Serial.available()){  //Read data to filename
    filename[i] = Serial.read();  
    Serial.print(filename[i]);
    i++; 
    delay(10); //wait for more data
  }

  Serial.print("\n\rFilename: ");  
  Serial.println(filename);
  //Create the file
  dataFile = SD.open(String(filename), FILE_WRITE);
  dataFile.close();
}

void loop() {
  // put your main code here, to run repeatedly:
  dataFile = SD.open(String(filename), FILE_WRITE);
  while (Serial.available()){
    dataFile.print((char) Serial.read());
    Serial.print(".");
  }
  dataFile.close();
}
