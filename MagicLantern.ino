
//----------------------------------------------------------------------------------
//-- MODIFY THESE

// how many tracks we will have — use this as a backup system in case we see a 
// repetitive 9999 error
const int defaultNumTracks = 5;

// how many times we will flash the lamp on startup, use 0 for no flashing
const int lampCycles = 3;

// time in milliseconds after stepping on the pad that the sound will trigger
const int steppedOnMatWaitTime = 6000; 

// how long we will wait for sound to finish playing
const int soundPlaybackTime = 15000;      

//-- how long we will flicker the lamp on/off after sound playback
const int lampFlickerTime = 5000;        

//-- set to random on/off, with low and high values  
const int minLampOffTime = 100;
const int maxLampOffTime = 400;
const int intitialLampOffTime = 750;

   

//----------------------------------------------------------------------------------

//-- 1 = send debug messages to the serial port, 0 = no debug (faster)
#define SERIAL_DEBUG (1)

//-- standard libs
#include <SoftwareSerial.h>
#include <Wire.h> 

//-- 3rd party libs
#include "MP3Trigger.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

//-- my libs
#include "MSTimer.h"

//-- general: wait times
int debounceMS = 100;
int startupDelayTime = 2500;      // how long of a delay when we startup

//-- all times
MSTimer soundPlaybackTimer;       // how long we will consider the sound to be playing
MSTimer steppedOnMatTimer;        // how long after user steps on mat before we play sound
MSTimer lampFlickerTimer;         // how long to flicker the lamp for
MSTimer switchLampTimer;          // how long to switch lamp on/off to simulate speaking

//-- states
const int stateStartup = 0;
const int stateError = 1;
const int stateWaiting = 2;             // user off mat
const int stateSteppedOnMat = 3;        // user has just stepped on the mat
const int statePlaying = 4;             // sound is playing (user is on may or off)
const int stateStillOnMat = 5;          // user still on mat, sound is finished playing


int state       = stateStartup;       // current state
boolean lampOn  = true;               // whether lamp is on or off

//-- pins
const int rxSoftSerialPin = 2;
const int txSoftSerialPin = 3;
const int soundTriggerLED = 4;         // LED for the sound trigger
const int acRelayPin = 6;
const int acRelayTestPin = 8;           // momentary switch for AC relay
const int soundTriggerPin = 10;         // a momentary switch, used to trigger the sound for debugging
const int floorMatPin = 13;             // a floor mat, which acts as toggle switch


//-- tracks
int numTracks;                   // how many tracks we have
int trackNum = 1;                 // which track is the current track
int lastTrackPlayed;

//-- initialize software seriaal for MP3 player
// RX = 2 (connects to TX of MP3), TX = 3 (connects to RX of MP3)
SoftwareSerial trigSerial = SoftwareSerial(rxSoftSerialPin, txSoftSerialPin);

//-- initialize 7-segment display
Adafruit_7segment matrix = Adafruit_7segment();

//-- initialize MP3 player
MP3Trigger trigger;

void setup() {
  //-- Metro Mini needs a delay for the two devices to properly syn
  delay(startupDelayTime);
  
  //-- initial
  state = stateStartup;
  
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
  pinMode(acRelayPin, OUTPUT);

  digitalWrite(soundTriggerLED,LOW);
  
  // Start serial communication with the trigger (over Soft Serial)
  //-- Use 19200 baud, note this requires a .INI file
  trigger.setup(&trigSerial);
  trigSerial.begin( 19200 );
 
  //-- get the number of tracks
  numTracks = getNumTracks();
  
  if( numTracks == -1 ) {
     state = stateError;
     matrix.print(9999);
     numTracks = defaultNumTracks;
  }
  else
      matrix.print(numTracks);
  
   matrix.writeDisplay();

  if( SERIAL_DEBUG ) {
    Serial.print("num tracks = " );
    Serial.print( numTracks );    
    Serial.println();
  }

  // do a quick on/off of the lamp when start up
  cycleLamp(lampCycles,150);

    state = stateWaiting;

    soundPlaybackTimer.setTimer(soundPlaybackTime);
    steppedOnMatTimer.setTimer(steppedOnMatWaitTime);
    lampFlickerTimer.setTimer(lampFlickerTime);
      
    lastTrackPlayed = -1;
}

void loop() {
  boolean triggerSound = false;
  boolean momentaryTrigger = false;
  
  if( checkFloorMatSoundTrigger())
     triggerSound = true;

  // test pin for momentary ac relay swith
   if( momentaryButtonPressed(acRelayTestPin)) {
      turnLampOff();
      delay(250);
      turnLampOn();
   }
   
   // test pin for momentary sound trigger switch
  if( momentaryButtonPressed(soundTriggerPin) )  {
    momentaryTrigger = true;
    triggerSound = true;
  }
  
  if( triggerSound ) {
      while(true) {
        // this seems to work better than the random number generator
        trackNum = millis() % numTracks + 1;
        if( trackNum != lastTrackPlayed ) {
           lastTrackPlayed = trackNum;
            break;
        }
      }

     trigger.setVolume(0x00);
      trigger.trigger(trackNum);
      matrix.print(trackNum);
      matrix.writeDisplay();

       if( SERIAL_DEBUG ) {
        Serial.print("Triggering track #");
        Serial.print(trackNum);
        Serial.println();
        }
  
      // quick feedback with LED
      if(  momentaryTrigger ) {
        digitalWrite(soundTriggerLED, HIGH);
        delay(1000);
        digitalWrite(soundTriggerLED,LOW);
      }
  }

  // green LED reflects the state (not what the mat is doing)
  if( state == statePlaying )
    digitalWrite(soundTriggerLED,  HIGH );
    
  else if( state == stateSteppedOnMat ) {
    digitalWrite( soundTriggerLED, HIGH );
    delay(100);
     digitalWrite( soundTriggerLED, LOW );
     delay(100);
  }
  
  else
    digitalWrite( soundTriggerLED, LOW );
}

void cycleLamp(int numCycles, int delayTime) {
    //-- do a short activation sequence with the lightbulb (for now)
    for( int i = 0; i < numCycles; i++ ) {
      turnLampOff();
      delay(delayTime);
      turnLampOn();
      delay(delayTime);
    }
}
    
//-- return true if momentary button is pressed, manages debounce, we only care about the ON state
boolean momentaryButtonPressed(int switchPin) {
 //-- check the momentary sound trigger pin
  if( digitalRead(switchPin) == true  ) {   // button is pressed
        delay(debounceMS); // debounce
         
         // wait for button to be released
         while( digitalRead(soundTriggerPin) == true ) 
           ;  // do nothing
           
         delay(debounceMS); // debounce
    return true;
  }

  return false;
}
         
//-- acts as a toggle switch, return TRUE if we are to play a sound
//-- condition for this to happen:
//-- (1) visitor has just stepped on the mat AND the mat wait timer expired, return TRUE

boolean checkFloorMatSoundTrigger() {
   //-- we are on the mat, but haven't triggered the sound yet
  if( state == stateSteppedOnMat ) {
       // check to see if we stepped off the mat before the sound trigger
      if( digitalRead(floorMatPin) == false ) {
          state = stateWaiting;

          if( SERIAL_DEBUG ) 
            Serial.println("state = WAITING");
         
      }
          
      //-- still on mat, check mat-stepped on timer
      else if( steppedOnMatTimer.isExpired() ) {
        
        soundPlaybackTimer.start();

        //-- we will begin flickering the lamp to simulate "speaking"
        lampFlickerTimer.start();
        
        //-- switch lamp (this will be to OFF)
        turnLampOff();
        switchLampTimer.setTimer(intitialLampOffTime);
        switchLampTimer.start();
        
        state = statePlaying;
        
         if( SERIAL_DEBUG ) 
            Serial.println("state = PLAYING");
         
        return true;
      }
  }
  
  //-- still playing, check the playack timer
  if( state == statePlaying ) {
    if( soundPlaybackTimer.isExpired() == true ) {
      if( digitalRead(floorMatPin) == true ) {
        state = stateStillOnMat;
        if( SERIAL_DEBUG ) {
            Serial.println("state = STILL ON MAT");
         }
      }
      else {
        state = stateWaiting;
        if( SERIAL_DEBUG ) {
            Serial.println("state = WAITING");
         }
      }
    }

    else {
        //-- not expired, check to see if we should switch the lamp on/off
        if( lampFlickerTimer.isExpired() == false ) {

          // switchLamp() will also reset the timer
          if( switchLampTimer.isExpired() )
            switchLamp();
        }
        else {
          //-- make sure lamp goes on, if it was off and lamp just  stopped flickering
          if( lampOn == false )
            turnLampOn();
        }
    }
  }

  if( state == stateStillOnMat ) {
    if( digitalRead(floorMatPin) == false ) {
      delay(debounceMS); // debounce
      state = stateWaiting;

      if( SERIAL_DEBUG )
        Serial.println("state = WAITING");
      
    }
  }
  //-- check to see if we have stepped on the mat
  if( state == stateWaiting ) {
    // Just stepped on mat, start timer, switch states
    if( digitalRead(floorMatPin) == true ) {
      state = stateSteppedOnMat;

      if( SERIAL_DEBUG )
         Serial.println("state = STEPPED ON MAT");
      
      steppedOnMatTimer.start();  
    }
  }

     
  return false;
}


//-- AC Relay Functions, inverted since relay is normally-closed
void turnLampOn() {
   digitalWrite(acRelayPin, false); 
   lampOn = true;
}

void turnLampOff() {
   digitalWrite(acRelayPin, true); 
   lampOn = false;
}

//-- switches lamp on or off, based on current state and will spwan the switch lamp timer
void switchLamp() {
  if(lampOn)
    turnLampOff();
  else
    turnLampOn();

  switchLampTimer.setTimer(random(minLampOffTime,maxLampOffTime));
  switchLampTimer.start();
}

//-- send num tracks query
//-- might not work for more than 99 tracks
//-- returns -1 if error
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

      return -1;
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

    //Serial.println("incr"); 
    byteCount++;
  }

  //-- now, reverse-process the array,

  //-- count num digits
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

      //-- ugly debug code, no longer needed, ucomment just in case
      /*
      Serial.print("digit = ");
      Serial.print(numArray[i]);
      Serial.println(""); 
      Serial.print("Num = ");
      Serial.print(num);
      Serial.println(""); 
      */
      
      decimalPlaces++;
    }
  }
 
  //-- there is a bug in the MP3 board, where it reports back DOUBLE the number of tracks available
  //-- so, we divide by 2 to compensate
  return num/2;
}
  

