
//----------------------------------------------------------------------------------
//-- MODIFY THESE

// time in milliseconds after stepping on the pad that the sound will trigger
const int soundTriggerDelayTime = 1000;       

//----------------------------------------------------------------------------------

#include <SoftwareSerial.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
Adafruit_7segment matrix = Adafruit_7segment();



//-- 1 = send debug messages to the serial port, 0 = no debug (faster)
#define SERIAL_DEBUG (1)

#include <MP3Trigger.h>

//-- general: wait times
int debounceMS = 100;

//-- pins
int soundTriggerPin = 12;         // a momentary switch, used to trigger the sound for debugging
int floorMatPin = 11;             // a floor mat, which acts as toggle switch
int soundTriggerLED = 4;         // LED for the sound trigger

int acRelayTestPin = 9;           // momentary switch for AC relay
int acRelayLEDPin = 8;            // LED for the AC relay, displays ON when the circuit is closed

//-- states
#define STATE_READY           (0)         // ready to play a sound
#define STATE_STANDING        (1)         // person is standing on mat

//-- MP3 player
MP3Trigger trigger;
unsigned long waitTime = 1700;
unsigned long startTime = 0;
int  numTracks = 17;
int trackNum = 1;

// RX = 2 (connects to TX of MP3)
// TX = 3 (connects to RX of MP3)
SoftwareSerial trigSerial = SoftwareSerial(2, 3);

void setup() {
  if( SERIAL_DEBUG ) {
    Serial.begin(9600);
    Serial.println("Magic Lantern, starting up");
  }

  //-- initialize the 4-segment number display
  matrix.begin(0x70);
  matrix.print(0);
  matrix.writeDisplay();
  
  //-- initalize various pins, specify INPUT pins for code legibiity
  pinMode(soundTriggerPin, INPUT); 
  pinMode(floorMatPin, INPUT); 
  pinMode(acRelayTestPin, INPUT); 
  
  pinMode(soundTriggerLED, OUTPUT); 
  pinMode(acRelayLEDPin, OUTPUT); 
  

  digitalWrite(soundTriggerLED,LOW);
  
  // Start serial communication with the trigger (over Soft Serial)
  //-- Use 19200 baud, note this requires a .INI file
  trigger.setup(&trigSerial);
  trigSerial.begin( 19200 );
  delay(1000);


  
  //-- get the number of tracks
  numTracks = getNumTracks();
  numTracks = 6;
  
  if( numTracks == -1 ) {
     matrix.print(9999);
     numTracks = 0;
  }
  else
      matrix.print(numTracks);
  
   
   matrix.writeDisplay();

  if( SERIAL_DEBUG ) {
    Serial.print("num tracks = " );
    Serial.print( numTracks );    
    Serial.println();
  }
  
   //-- seed random number generator
   randomSeed(A0);

   
}

void loop() {
  // process signals from the trigger [OBSOLETE?]
  //trigger.update();

  //-- tester, don't use with button
  //playRandomTrack();

  if( triggerSound() ) {
      trackNum = random(numTracks) + 1;
      trigger.trigger(trackNum);
      matrix.print(trackNum);
      matrix.writeDisplay();

      
      digitalWrite(soundTriggerLED, HIGH);
      delay(1000);
      digitalWrite(soundTriggerLED,LOW);
  }
}

//-- return true if we are to trigger a sound, false if not; will change the states accordingly
boolean triggerSound() {
  //-- check to see if the floor mat has been triggered
  //-- [code to go here]
  
  //-- check the momentary sound trigger pin
  if( digitalRead(soundTriggerPin) == true  ) {   // button is pressed
        delay(debounceMS); // debounce
         
         // wait for button to be released
         while( digitalRead(soundTriggerPin) == true ) {
           ;  // do nothing
         }
         delay(debounceMS); // debounce
    return true;
  }

  return false;
}

//-- OBSOLETE CODE
void playRandomTrack() {
  if( startTime + waitTime < millis() ) {
    trackNum = random(numTracks) + 1;
    trigger.trigger(trackNum);
    matrix.print(trackNum);
    matrix.writeDisplay();

  
    trackNum++;
    if( trackNum > numTracks )
      trackNum = 1;
    startTime = millis();
  }
}


//-- send num tracks query
//-- fix, won't work for more than 99 tracks
//-- also, gives
//-- return -1 if error
int getNumTracks() {
  //-- an array of numerical entries, which will be reverse-ordered
  int numArrayEntries = 4;
  int numArray[numArrayEntries];        // max 3 entries on this MP3 card, we will do 4 just in case
  for( int i = 0; i < numArrayEntries; i++ ) {
    numArray[i] = -1;   // flag
  }
  
  //-- make request
  trigSerial.flush();
  //trigSerial.println("S1");
  
  trigSerial.write('S');
  trigSerial.write('1');
  delay(1000);
  
  int byteCount = 0;      //-- How many bytes we have counted, use for multiplier, 
  
  if( SERIAL_DEBUG ) {
    Serial.println("getNumTracks()");
  }

   if( !trigSerial.available() ) {
      if( SERIAL_DEBUG ) {
          Serial.println("no software serial response");
      }
  }
  
  while( trigSerial.available() )
  {
    int incomingByte = trigSerial.read();

    //-- first byte should be ASCII 61
    if( byteCount == 0 ) {
      if (incomingByte != 61 ) {
        if( SERIAL_DEBUG )
          Serial.print("ERROR: expecting first byte of 61");

         return -1;
      }
    }

    //-- next bytes are actual numbers
    else {
      // shift over that many decimal places
      incomingByte = incomingByte - 48;

      if( SERIAL_DEBUG ) {
        Serial.println();
        Serial.print(incomingByte, DEC);
      }
      
     // crude convert to int
      Serial.print(byteCount, DEC);
      Serial.println();
      
      //-- check for array overflow (shoudn't happen)
      int numArrayIndex = byteCount - 1;
      if( numArrayIndex > (numArrayEntries-1) )  {
        if( SERIAL_DEBUG )
          Serial.print("ERROR: more tracks than expected");
         return -1;
      }

      // how many digits we have processed: 10 to that power
      numArray[numArrayIndex] = incomingByte;
    
      if( SERIAL_DEBUG ) { 
           Serial.print(numArray[numArrayIndex], DEC);
           Serial.println();
      }
     
    }

    Serial.println("incr"); 
    byteCount++;
  }

  //-- now, reverse-process the array,

  //-- count num digits
  Serial.println("reverse-processing");
  int num = 0;            //-- number of tracks, aggregating
  int decimalPlaces = 0;
  for( int i = numArrayEntries-1; i >= 0; i-- ) {
  //for( int i = numArrayEntries-1; i >= 0; i-- ) {
          
    if( numArray[i] != -1 ) {
      if( decimalPlaces == 0 )
        num = numArray[i];
      else {
          int mutiplier = 1;
          for( int j = 0; j < decimalPlaces; j++ ) 
            mutiplier *= 10;
            
          
          num +=  (numArray[i] * mutiplier );      
      }
      
      Serial.print("digit = ");
      Serial.print(numArray[i]);
      Serial.println(""); 
      Serial.print("Num = ");
      Serial.print(num);
      Serial.println(""); 
      
      decimalPlaces++;
    }
  }
 
  //-- there is a bug in the MP3 board, where it reports back DOUBLE the number of tracks available
  //-- so, we divide by 2 to compensate
  return num/2;
}
  

