#include <ArduinoLowPower.h>

#include <Servo.h>

#include <WiFiNINA.h>
#include <RTCZero.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
//  https://create.arduino.cc/projecthub/doug-domke/self-setting-super-accurate-clocks-5f1162
//  The code for the clocks are taken from the project Self-Setting Super Acccurate Clocks
//  Link abve.
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////

Servo myServo;  // define servo variable name
RTCZero rtc;    // create instance of real time clock


// clock variables
int status = WL_IDLE_STATUS;
int myhours, mins, secs, myday, mymonth, myyear, dayofweek;
bool IsPM = false;
boolean progMove = false;         // not currrently using

//int moveHour, moveMin;
//int movePosition = BLIND_UP;          // variable to store the servo position (in degrees)
const int MOVE_ROWS = 6;                // number of blind alarms
const int MOVE_COLS = 4;                // number of vars needed for blind alarm (Day, Hour, Min, Position) of move
const int MOVE_DAY = 0;                 // column position of day in array moveInfo
const int MOVE_HOUR = 1;                // column position of hour in array moveInfo
const int MOVE_MIN = 2;                 // column position of minutes in array moveInfo
const int MOVE_POSITION = 3;            // column position of degrees to move to in array moveInfo
int moveInfo[MOVE_ROWS][MOVE_COLS];     // arrray to hold the moveDay, moveHour, moveMin, movePosition 
int currentPosition;                    // variable to store the servo position (in degrees)
boolean moveIsSet = false;              // allows the user to turn off all alarms at once
boolean movingUp = true;                // set direction of movement, moving up or not (which is down)
                                        

// USER SETTINGS
char ssid[] = "silverorbi";       // My WiFi network
char pass[] = "favor1201breezy";  // My WiFi password
const int GMT = -7;               // change this to adapt it to your time zone, PST for seattle
const int myClock = 24;           // can be 24 or 12 hour clock, use 24 when moving blinds
const int dateOrder = 1;          // 1 = MDY; 0 for DMY
// End USER SETTINGS

// set up pins on Arduino to for servo & push button
int servoPin = 9;                 // choose the pin for the servo, must be 9 or 10
//int inputPin = 6;                 // connect sensor to input pin 6 (push button)

// constants for degrees for the position of the blind & how quickly blinds move
const int BLIND_UP = 0;               // blind is up
const int BLIND_QUARTER_UP = 45;      // blind is pointing up 45 deg
const int BLIND_HALF = 90;            // blind is open
const int BLIND_QUARTER_DOWN = 135;   // blind is pointing down 45 deg
const int BLIND_DOWN = 180;           // blind is down
const int MOVE_DELAY = 100;           // time delay when moving blinds
const int MOVE_STEP = 45;             // the number of degrees to move at each step


void setup() {

  Serial.begin (115200); 

  // set up blinds to be up (0 degrees)
  myServo.attach(servoPin);       // select servo pin 
  currentPosition = BLIND_UP;     // set position of blinds to 0 degrees
  myServo.write(currentPosition); // physically set blinds to 0 degrees
  
//  pinMode(servoPin, OUTPUT);      // set servo as output
//  pinMode(inputPin, INPUT);       // set pushbutton as input
 
  // set up RTC
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }
  delay(6000);
  rtc.begin();
  setRTC();                   // get Epoch time from Internet Time Service
  setToMove();                // set up blind alarms
  fixTimeZone();
  
  Serial.print("currentPostion of blind: ");
  Serial.println(currentPosition);
  printDate();
  printTime();
  Serial.println();
}

void loop() { 
  secs = rtc.getSeconds();
  if (secs == 0) fixTimeZone();                 // when secs is 0, update everything and correct for time zone
  // otherwise everything else stays the same.
 
  if (mins==59 && secs ==0) setRTC();           // get NTP time every hour at minute 59 


  checkToMove();                                // check to see if it's time to move the blinds 

}         

///////////////////////////////////////////////////////////////////
// Get the time from Internet Time Service
///////////////////////////////////////////////////////////////////
void setRTC() { 
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();                   // The RTC is set to GMT or 0 Time Zone and stays at GMT.
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {
    Serial.println("NTP unreachable!!");
    while (1);  // hang
  }
  else {
    dayofweek = ((epoch / 86400) + 4) % 7;    // set day of week 0-6 = Sun-Sat ****************************
    Serial.print("Epoch Time = ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);
    Serial.println();
  }
}

/////////////////////////////////////////////////////////////////////////////
// There is more to adjusting for time zone than just changing the hour.
// Sometimes it changes the day, which sometimes chnages the month, which
// requires knowing how many days are in each month, which is different
// in leap years, and on Near Year's Eve. It can even change the year! 
/////////////////////////////////////////////////////////////////////////////
void fixTimeZone() {
  int daysMon[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (myyear % 4 == 0) daysMon[2] = 29;      // fix for leap year
  myhours = rtc.getHours();
  mins = rtc.getMinutes();
  myday = rtc.getDay();
  mymonth = rtc.getMonth();
  myyear = rtc.getYear();
  myhours +=  GMT;                        // initial time zone change is here
  if (myhours < 0) {                      // if hours rolls negative
    myhours += 24;                        // keep in range of 0-23
    myday--;  // fix the day
    if (myday < 1) {                      // fix the month if necessary
      mymonth--;
      if (mymonth == 0) mymonth = 12;
      myday = daysMon[mymonth];
      if (mymonth == 12) myyear--;         // fix the year if necessary
    }
  }
  if (myhours > 23) {                      // if hours rolls over 23
    myhours -= 24;                         // keep in range of 0-23
    myday++;                               // fix the day
    if (myday > daysMon[mymonth]) {        // fix the month if necessary
      mymonth++;
      if (mymonth > 12) mymonth = 1;
      myday = 1;
      if (mymonth == 1)myyear++;           // fix the year if necessary
    }
  }
  if (myClock == 12) {                     // this is for 12 hour clock
    IsPM = false;
    if (myhours > 11)IsPM = true;
    myhours = myhours % 12;                // convert to 12 hour clock
    if (myhours == 0) myhours = 12;        // show noon or midnight as 12
  }
}


//////////////////////////////////////////////////////////////
// Print out time & date correctly
//////////////////////////////////////////////////////////////
void printDate()
{
  if (dateOrder == 0) {
    Serial.print(myday);
    Serial.print("/");
  }
  Serial.print(mymonth);
  Serial.print("/");
  if (dateOrder == 1) {
    Serial.print(myday);
    Serial.print("/");
  }
  Serial.print("20");
  Serial.print(myyear);
  Serial.print(" ");
}

void printTime()
{
  print2digits(myhours);
  Serial.print(":");
  print2digits(mins);
  Serial.print(":");
  print2digits(secs);
  if (myClock==12) {
    if(IsPM) Serial.print("  PM");
    else Serial.print("  AM");
  }
  Serial.println();
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}



//////////////////////////////////////////////////////////////////////////////
// This routine sets/gets hours and minutes for when to move the blinds
// 
//////////////////////////////////////////////////////////////////////////////
void setToMove() { 
  moveIsSet = true;   // hard code that we are going to move the blinds

// hard code alarms for now, up to 6 blind alarms
  moveInfo[0][0] = 1;  // day, 0-6 = Sun-Sat
  moveInfo[0][1] = 15;  // hour, 24hr
  moveInfo[0][2] = 33;  // min
  moveInfo[0][3] = 90;  // position, 0-180 deg

  moveInfo[1][0] = 1;  // day, 0-6 = Sun-Sat
  moveInfo[1][1] = 15;  // hour, 24hr
  moveInfo[1][2] = 34;  // min
  moveInfo[1][3] = 135;  // position, 0-180 deg

  moveInfo[2][0] = 1;  // day, 0-6 = Sun-Sat
  moveInfo[2][1] = 15;  // hour, 24hr
  moveInfo[2][2] = 35;  // min
  moveInfo[2][3] = 90;  // position, 0-180 deg

  moveInfo[3][0] = 1;  // day, 0-6 = Sun-Sat
  moveInfo[3][1] = 15;  // hour, 24hr
  moveInfo[3][2] = 36;  // min
  moveInfo[3][3] = 95;  // position, 0-180 deg

  moveInfo[4][0] = 1;  // day, 0-6 = Sun-Sat
  moveInfo[4][1] = 15;  // hour, 24hr
  moveInfo[4][2] = 37;  // min
  moveInfo[4][3] = 170;  // position, 0-180 deg

  moveInfo[5][0] = 1;  // day, 0-6 = Sun-Sat
  moveInfo[5][1] = 15;  // hour, 24hr
  moveInfo[5][2] = 38;  // min
  moveInfo[5][3] = 0;  // position, 0-180 deg

 // fixTimeZone(); // Make sure minutes are up to date on exit
 
}

//////////////////////////////////////////////////////////////////////////////
// If myhours and mins match time to move & moveIsSet is true &
// if blinds are not in desired position then move them to desired position
//////////////////////////////////////////////////////////////////////////////
void checkToMove() {                  
  if (moveIsSet == true) {
    for (int i=0; i<MOVE_ROWS; i++) {
      if ((dayofweek == moveInfo[i][MOVE_DAY]) && (myhours == moveInfo[i][MOVE_HOUR]) && (mins == moveInfo[i][MOVE_MIN])) {
        if (currentPosition != moveInfo[i][MOVE_POSITION]) {
          tiltBlinds(moveInfo[i][MOVE_POSITION]);     // move the blinds as needed
        }
      }
    }
    fixTimeZone();    // make sure minutes stay up to date 
  }
}

////////////////////////////////////////////////////////////////
// Move the blinds 1 degree at a time with a delay after each  
// move. The blinds should move between 0 and 180 degrees.
/////////////////////////////////////////////////////////////////  
void tiltBlinds(int movePosition) {   
  if (currentPosition < movePosition) {           // move blind up
    for (int i = currentPosition+1; i <= movePosition; i++) {
          myServo.write(i);                       // set rotating angle of the motor to i degrees
          delay(MOVE_DELAY);                      // slow the process  
    }
  } else if (currentPosition > movePosition) {    // move blind down
    for (int i = currentPosition-1; i >= movePosition; i--) {
          myServo.write(i);                       // set rotating angle of the motor to i degrees
          delay(MOVE_DELAY);                      // slow the process  
    }
  }
  
  Serial.println(currentPosition);    // for debug
  Serial.println(movePosition);
  printDate();
  printTime();
  Serial.println();                   // end for debug

  currentPosition = movePosition;                   // set the new currentPosition of the blinds
}
