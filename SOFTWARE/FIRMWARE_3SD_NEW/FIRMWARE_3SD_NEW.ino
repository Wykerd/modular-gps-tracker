/*  Modular GSM GPS Tracker Firmaware 3A
 *  Software by Daniel Wykerd
 *  
 *  Features:
 *  - Auto SMS receive and loading into character arrays
 *  - Responds to the commands #HELP and #LOCATION
 *  
 *  This software is made to be used with REV 1.0 of the core module
 */

#include <String.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <AltSoftSerial.h>
SoftwareSerial sms = SoftwareSerial(2, 3);
SoftwareSerial sd = SoftwareSerial(11, 12);
AltSoftSerial ssgps;

TinyGPSPlus gps;

char atResponse[120];
char notificationBuf[80];
char senderNum[20];
char msg[20];
boolean smsFound = false;
bool record = true;

const char format = ".GPX";

void setup() {
  // put your setup code here, to run once:
  sd.begin(9600);
  sms.begin(9600);
  ssgps.begin(9600);
  Serial.begin(115200);
  while(!networkStatus()); //Do nothing while there is no network
  sendAT("AT+CMGF=1");
  sendAT("AT+CNMI=2,1");
  deleteSMS();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); 
  Serial.println(F("Boot Successful"));
  record = false;
  while (sms.available() > 0) sms.read(); //Clear the buffer before starting to prevent data loss in first sms
}

void deleteSMS(){
  sendAT("AT+CMGD=1,4");
}

void startSMS(){
  sms.print("AT+CMGS=\"" + String(senderNum) + "\"\r");
  delay(100);
}

void sendSMS(){
  sms.write(0x1A);
  sms.write(0x0D);
  sms.write(0x0A); 
  Serial.print(F("\nSMS Sent!"));
}

uint8_t preSec;

void loop() {
  preSec = gps.time.second();
  parseGPS();
  // put your main code here, to run repeatedly:
  checkForSMS();
  if (smsFound) {
    smsFound = false;
    Serial.print(F("From: "));
    Serial.print(senderNum);
    Serial.print(F("\nMessage: "));
    Serial.print(msg);
    String sMsg = String(msg);
    sMsg.toUpperCase();
    if (msg[0] == '#') {
      if (sMsg.indexOf('HELP') > 0) {
        startSMS();
        sms.print(F("HELP\n"));
        sms.print(F("Dev Build 3SD"));
        sms.print(F("\n  #LOCATION - Sends current location"
                    "\n  #RECORD - Logs location to SD Module"));
        sendSMS();  
      }else if (sMsg.indexOf('LOCATION') > 0) {
        startSMS();
        if (gps.satellites.value() >= 3) {
          //Sends location
          sms.print(F("Location: "));
          sms.print(gps.location.lat(), 6);
          sms.print(F(", "));
          sms.print(gps.location.lng(), 6);
          //Sends link to Google Maps
          sms.print(F("\nGoogle maps: "));
          sms.print(F("google.com/maps/place/"));
          sms.print(gps.location.lat(), 6);
          sms.print(F(","));
          sms.print(gps.location.lng(), 6);
          //Send amount of satellites for debugging
          sms.print(F("\nSatellites: "));
          sms.print(gps.satellites.value());
        } else {
          sms.print(F("Acquiring satellites, try later"));
        }
        sendSMS();
      }else if (sMsg.indexOf('RECORD') > 0) {
        recordCommand();
      }
    }
  }
  
  char ftime[20];
  sprintf(ftime, "%04d-%02d-%02dT%02d:%02d:%02dZ", gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());  
  
  if (((record)&&(preSec != gps.time.second()))&&(gps.satellites.value() >= 3)) {
    //Add to log
    sd.print("<trkpt lat=\"");
    sd.print(gps.location.lat(), 6);
    sd.print("\" lon=\"");  
    sd.print(gps.location.lng(), 6);
    sd.print("\">\n\r<time>");
    delay(100);
    sd.print(ftime);
    sd.print("</time>\n\r<ele>");
    sd.print(gps.altitude.meters());
    delay(100);
    sd.println("</ele></trkpt>");
    delay(100); 
  }
}

void recordCommand(){
  record = !record;
  startSMS();
  if (record){
    startGPX();
    sms.print(F("Started Recording"));
  }
  else{
    endGPX();
    sms.print(F("Ended Recording"));
  }
  sendSMS();
}

void startGPX(){
  digitalWrite(13, HIGH);
  delay(5000);
  while (gps.satellites.value() < 3) parseGPS();
  sd.print(gps.date.value());
  sd.print(gps.time.hour());
  sd.print(".GPX");
  delay(200);
  sd.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  delay(50);
  sd.print("<gpx version=\"1.1\" xmlns=\"http://www.topograf");
  delay(50);
  sd.print("ix.com/GPX/1/1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-in");
  delay(50);
  sd.print("stance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1");
  delay(50);
  sd.println(" http://www.topografix.com/GPX/1/1/gpx.xsd\">");
  delay(50);
  sd.println("<trk><trkseg>");
  delay(100);
}

void endGPX() {
  sd.print("</trkseg>\n\r</trk>\n\r</gpx>");
  delay(1000);
  digitalWrite(13, LOW);
}

void parseGPS(){
  while (ssgps.available() > 0) gps.encode(ssgps.read());
}

void getSMSDat(){
  sendAT("AT+CMGR=1");
  if ((strstr(atResponse, "+CMGR:")) != NULL){ //Make sure the response is correct
    sscanf(atResponse, "%[^,],\"%[^\"]\"%[^\r\n]\r\n%[^\r\n]", NULL, senderNum, NULL, msg); //PARSE THE DATA
    deleteSMS(); //CLEAR THE MODEM MEMORY!
  }
}

void checkForSMS(){
  uint8_t i = 0;
  memset(notificationBuf, '\0', 100);
  while ((sms.available() != 0) && (i < 255)){
    notificationBuf[i] = sms.read();
    Serial.print(notificationBuf[i]);
    i++; 
  }
  delay(100);
  if ((strstr(notificationBuf, "+CMTI")) != NULL){
    getSMSDat();
    smsFound = true;
  }
}

boolean networkStatus(){
  sendAT("AT+CGREG?");

  //parse the response
  if ((strstr(atResponse, "+CGREG:")) != NULL){ //Make sure the response is correct
    strtok(atResponse," :,");
    strtok (NULL," :,");  
    if ((strstr ((strtok (NULL," :,")),"1")) != NULL) return true; 
    else return false;
    
  } else return false; 
}

void sendAT(char* atCommand){
  uint8_t i=0;
  memset(atResponse, '\0', 100); //Clear the char array
  while( sms.available() > 0) sms.read();
  sms.println(atCommand);     //Print the at command
  Serial.print(F("<- "));
  Serial.println(atCommand);
  delay(100);
  unsigned long initialTime = millis();
  do{
    if (sms.available() != 0){
      atResponse[i] = sms.read();
      i++; 
    }
  } while((strstr(atResponse, "OK") == NULL) && (millis() - initialTime)<2000); //Keep searching for response for 2 sec or until "OK" which indicates the end of a AT response
  Serial.print(F("-> "));Serial.println(atResponse);
}

