
#include <SoftwareSerial.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
Adafruit_7segment matrix = Adafruit_7segment();


#include <MP3Trigger.h>
MP3Trigger trigger;
unsigned long waitTime = 1700;
unsigned long startTime = 0;
int  numTracks = 17;
int trackNum = 1;

SoftwareSerial trigSerial = SoftwareSerial(2, 3);

void setup() {
  Serial.begin(9600);
  matrix.begin(0x70);
  matrix.print(0);
  matrix.writeDisplay();
  
  // Start serial communication with the trigger (over Serial)
  // hard serial
  
  trigger.setup(&trigSerial);
  trigSerial.begin( 19200 );

  //trigger.setup(&Serial);
  //Serial.begin( MP3Trigger::serialRate() );
  // Start looping TRACK001.MP3
  //trigger.setLooping(true,1);
  randomSeed(A0);

  numTracks = getNumTracks();
}

void loop() {
  // process signals from the trigger
  trigger.update();



  if( startTime + waitTime < millis() ) {
    trackNum = random(numTracks);
    trigger.trigger(trackNum);
    matrix.print(trackNum);
    matrix.writeDisplay();

  
    trackNum++;
    if( trackNum > numTracks )
      trackNum = 1;
    startTime = millis();
  }
}


/*

void loop() {
 
  // put your main code here, to run repeatedly:

  // send data only when you receive data:
  if (Serial.available() > 0)
    readMP3Data();        
}


void readMP3Data() {
  numTracks = Serial.read();

matrix.print(numTracks);
  matrix.writeDisplay();
  // say what you got:
//  Serial.print("I received: ");
 // Serial.println(incomingByte, DEC);
        
}
*/
//-- send num tracks query
//-- fix, won't work for more than 99 tracks
//-- also, gives
int getNumTracks() {
  trigSerial.flush();
  trigSerial.write('S');
  trigSerial.write('1');
  delay(100);

  int count = 0;
  int num = 0;
  
   Serial.println("getting num tracks");
  while( trigSerial.available() )
  {
    int incomingByte = trigSerial.read();
    if( count == 0 ) {
      if (incomingByte != 61 ) {
        Serial.print("ERROR");
      }
      
      //break;
    }
    else {
      // shift over that many decimal places
      incomingByte = incomingByte - 48;

Serial.println();
      Serial.print(incomingByte, DEC);

     // crude convert to int
      Serial.print(count, DEC);
      num = num * (10* (count-1));
      num += incomingByte;


       Serial.println();
           Serial.print(num, DEC);
           
        Serial.println();
    }

    Serial.println("incr");
    count++;
  }

  return num/2;
}
  

