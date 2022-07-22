#include <Servo.h>

//#include <SPI.h>
#include <WiFiNINA.h>
#include <RTCZero.h>


Servo myServo;  // define servo variable name
RTCZero rtc;    // create instance of real time clock

// clock variables
int status = WL_IDLE_STATUS;
int myhours, mins, secs, myday, mymonth, myyear;
bool IsPM = false;

// USER SETTINGS
char ssid[] = "silverorbi";       // Your WiFi network
char pass[] = "favor1201breezy";  // Your WiFi password
const int GMT = -7;               // change this to adapt it to your time zone, DST for seattle
                                  // need to update automatically
const int myClock = 12;           // can be 24 or 12 hour clock
const int dateOrder = 1;          // 1 = MDY; 0 for DMY
// End USER SETTINGS

// set up pins on Arduino to for servo & push button
int servoPin = 9;                 // choose the pin for the servo, must be 9 or 10
int inputPin = 6;                 // connect sensor to input pin 6 (push button)
int pos = 0;                      // variable to store the servo position (in degrees)

// constants for degrees for the position of the blind & how quickly blinds move
const int BLIND_OPEN = 90;        // blind is open
const int HALF_CLOSED = 46;       // blind is half way closed
const int HALF_OPEN = 44;         // blind is half way open
const int BLIND_CLOSED = 0;       // blind is closed
const int MOVE_DELAY = 100;       // time delay when moving blinds


void setup() {
  // set up blinds
  myServo.attach(servoPin);   // select servo pin 
  pinMode(servoPin, OUTPUT);  // set servo as output
  pinMode(inputPin, INPUT);   // set pushbutton as input
  myServo.write(0);           // physically close blinds
  pos = CLOSED;               // set position of blinds to CLOSED

  // for debug
  Serial.begin (9600);  
 
  // set up RTC
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }
  delay(6000);
  rtc.begin();
  setRTC();  // get Epoch time from Internet Time Service
  fixTimeZone();
}

void loop() {
  
  secs = rtc.getSeconds();
  if (secs == 0) fixTimeZone(); // when secs is 0, update everything and correct for time zone
  // otherwise everything else stays the same.
 
  int val = digitalRead(inputPin);  // read input value
  if (val == LOW) {                 // check if the input is LOW, button is pushed in
    // decide where to move servo based on it's current position
    switch (pos)
    {

      //////////////////////////////////////////////////////
      // The blinds are open (at 90 degrees). 
      // Pressing the button closes the blinds halfway
      // (to a 45 degree angle).
      // The position variable, pos, will then be set to HALF_OPEN
      case BLIND_OPEN:
        for (int i = pos; i > HALF_OPEN; i -= 1) {
          // close the blinds 1 degree at a time 
          // with a delay after each move
          myServo.write(i);      // set rotating angle of the motor to i degrees
          delay(MOVE_DELAY);     // slow the process  
        }
        Serial.println("BLIND_OPEN");    // for debug
        printDate();
        printTime();
        Serial.println();
          
        pos = HALF_OPEN;
        break;
        
      //////////////////////////////////////////////////////
      // The blinds are half open (at 45 degrees). 
      // Pressing the button closes the blinds halfway
      // until the blinds are fully closed (0 degrees).
      // The postion variable, pos, will then be set to CLOSED.
      case HALF_OPEN:
        for (int i = pos; i >= BLIND_CLOSED; i -= 1) { 
          // close the blinds 1 degree at a time 
          // with a delay after each move
          myServo.write(i);    // set rotating angle of the motor to i degrees
          delay(MOVE_DELAY);   // slow the process  
        }
        Serial.println("HALF_OPEN");    // for debug
        printDate();
        printTime();
        Serial.println();
     
        pos = BLIND_CLOSED;
        break;
 
      //////////////////////////////////////////////////////
      // The blinds are half closed (at 45 degrees). 
      // Pressing the button opens the blinds halfway
      // until the blinds are fully open (90 degrees).
      // The postion variable, pos, will then be set to OPEN.
      case HALF_CLOSED:
        for (int i = pos; i <= BLIND_OPEN; i += 1) { 
          // open the blinds 1 degree at a time 
          // with a delay after each move
          myServo.write(i);    // set rotating angle of the motor to i degrees
          delay(MOVE_DELAY);    // slow the process  
        }
        Serial.println("HALF_CLOSED");    // for debug
        printDate();
        printTime();
        Serial.println();
       
        pos = BLIND_OPEN;
        break;


      /////////////////////////////////////////////////////
      // The blinds are closed (at 0 degrees). 
      // Pressing the button opens the blinds halfway
      // (to a 45 degree angle).
      // The position variable, pos, will then be set to HALF_CLOSED
      case BLIND_CLOSED:
        for (int i = pos; i < HALF_CLOSED; i += 1) {
          // open the blinds 1 degree at a time 
          // with a delay after each move
          myServo.write(i);    // set rotating angle of the motor to i degrees
          delay(MOVE_DELAY);    // slow the process  
        }
          Serial.println("BLIND_CLOSED");    // for debug
          printDate();
          printTime();
          Serial.println();

        pos = HALF_CLOSED;
        break;

    }     // switch   
  }       // if
  
  while (secs == rtc.getSeconds())delay(10);  // wait until seconds change
  if (mins==59 && secs ==0) setRTC();         // get NTP time every hour at minute 59 
  
}         // loop()

///////////////////////////////////////////////////////////////////
// Get the time from Internet Time Service
///////////////////////////////////////////////////////////////////
void setRTC() { 
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime(); // The RTC is set to GMT or 0 Time Zone and stays at GMT.
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);  // hang
  }
  else {
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
  if (myyear % 4 == 0) daysMon[2] = 29; // fix for leap year
  myhours = rtc.getHours();
  mins = rtc.getMinutes();
  myday = rtc.getDay();
  mymonth = rtc.getMonth();
  myyear = rtc.getYear();
  myhours +=  GMT; // initial time zone change is here
  if (myhours < 0) {  // if hours rolls negative
    myhours += 24; // keep in range of 0-23
    myday--;  // fix the day
    if (myday < 1) {  // fix the month if necessary
      mymonth--;
      if (mymonth == 0) mymonth = 12;
      myday = daysMon[mymonth];
      if (mymonth == 12) myyear--; // fix the year if necessary
    }
  }
  if (myhours > 23) {  // if hours rolls over 23
    myhours -= 24; // keep in range of 0-23
    myday++; // fix the day
    if (myday > daysMon[mymonth]) {  // fix the month if necessary
      mymonth++;
      if (mymonth > 12) mymonth = 1;
      myday = 1;
      if (mymonth == 1)myyear++; // fix the year if necessary
    }
  }
  if (myClock == 12) {  // this is for 12 hour clock
    IsPM = false;
    if (myhours > 11)IsPM = true;
    myhours = myhours % 12; // convert to 12 hour clock
    if (myhours == 0) myhours = 12;  // show noon or midnight as 12
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
