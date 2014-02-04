/* Swipe Remote Control
 
 This sketch uses an ultrasonic rangefinder to determine the user's gesture and outputs an IR signal to a sony TV based on the command given.
 - High swipe (> 10in) = Channel Up
 - Low swipe = Channel Down
 - High hold (> 10in) = Volume Up
 - Low hold = Volume Down
 - Cover sensor (< 3in) = Turn On / Off
 
 Created by Dan Nicholson.
 
 This example code is in the public domain.
 
 This code uses the IRremote library (https://github.com/shirriff/Arduino-IRremote)
 
 */
#include <IRremote.h>

// Defines for control functions
#define CONTROL_CH 1 // Channel change
#define CONTROL_VOL 2 // Volume
#define CONTROL_POW 3 // Power

#define CONTROL_UP 1
#define CONTROL_DOWN -1

#define DIST_MAX 20 // Maximum distance in inches, anything above is ignored.
#define DIST_DOWN 10 // Threshold for up/down commands. If higher, command is "up". If lower, "down".
#define DIST_POW 3 // Threshold for power command, lower than = power on/off

// IR PIN
const int irPin = 3; // this is defined in the library, this var is just a reminder. CHANGING THIS WILL NOT CHANGE PIN IN LIBRARY
// 2 Pin Ping Sensor
const int pingPin = 8;
const int echoPin = 7;
// Confirmation LED Pins
const int led = 13; //internal LED for up/down debugging
const int ledR = 11;
const int ledG = 10;
const int ledB = 9;
// LED on timer
unsigned long timer;
// IR transmitter object
IRsend irsend;
// Power confirmation flag (needs two swipes to send signal)
boolean powerConfirmed = false;

void setup() {
  // initialize serial communication and set pins
  Serial.begin(9600);
  pinMode(led, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  timer = millis();
}

void loop()
{

  //  Serial.println(millis());
  long duration, inches;
  int value;

  // Check for a reading
  duration = doPing();

  // Timer to confirm actions (currently only power)
  if (timer && timer < (millis() - 5000) && (millis() > 5000))
  {
    Serial.println("timer reset");
    timer = false;
  }

  digitalWrite(led, LOW);
  setColor(0, 0, 0); // off

  // convert the time into a distance
  inches = microsecondsToInches(duration);

  // If less than max inches away, act
  if (inches < DIST_MAX)
  {
    // Debug output
    Serial.print(inches);
    Serial.println("in");

    // If very close, it is a "power" signal
    if (inches < DIST_POW)
    {
      Serial.println(timer);
      // on or off
      if (timer)
      {
        doIR(CONTROL_POW, 0);
        timer = false;
        delay(2000); // don't want to be sending this more than once. 2 second delay
      }
      else
      {
        Serial.println("power flag set");
        timer = millis();
        setColor(255,50,50);
        delay(500);
      }
    }
    else // is volume or channel
    {
      // Distance determines control direction
      value = handleDist(inches);
      // wait half a second
      delay(300);
      // check again, has hand disappeared?
      if (microsecondsToInches(doPing()) > DIST_MAX)
      {
        doIR(CONTROL_CH, value); // swipe
      }
      else
      {
        // volume
        int d = 500; // first delay is longer for single volume change
        // repeat until hand is removed
        while (inches < DIST_MAX)
        {
          value = handleDist(inches); // is up or down?
          doIR(CONTROL_VOL, value); // fire off IR signal
          delay(d); // wait
          inches = microsecondsToInches(doPing()); // check for hand again
          d = 100; // delays are shorter for quick multiple volume adjustment
        }
        delay(500); // this stops accidental channel change after volume adjustment
      }
    }
  }
  delay(50); // Short enough to detect all swipes.
}
/*
* If distance is within threshold, mark as 'up' and turn on corresponding LED.
 */
int handleDist(int inches)
{
  if (inches > DIST_DOWN)
  {
    digitalWrite(led, HIGH);
    return CONTROL_UP;
  }
  else
  {
    digitalWrite(led, LOW);
    return CONTROL_DOWN;
  }
}

/*
* Fire off correct IR code
 */
void doIR(int control, int val)
{
  switch(control)
  {
  case CONTROL_POW:
    // power
    Serial.println("power on / off 0xa90");
    for (int i = 0; i < 3; i++)
    {
      setColor(255, 0, 0);
      irsend.sendSony(0xa90, 12); // Sony TV power code
      delay(40);
    }
    break;
  case CONTROL_CH:
    setColor(0, 255, 0);
    // output 'channel up / down' depending on val
    if (val == CONTROL_UP)
    {
      digitalWrite(led, HIGH);
      for (int i = 0; i < 3; i++)
      {
        irsend.sendSony(0x90, 12);
        delay(40);
      }
      Serial.println("channel up 0xD00A");
    }
    else // down
    {
      for (int i = 0; i < 3; i++)
      {
        irsend.sendSony(0x890, 12);
        delay(40);
      }
      Serial.println("channel down 0x3002");
    }
    break;
  case CONTROL_VOL:
    setColor(0, 0, 255);
    // output 'volume up / down' depending on val
    if (val == CONTROL_UP)
    {
      digitalWrite(led, HIGH);
      for (int i = 0; i < 3; i++)
      {
        irsend.sendSony(0x490, 12);
        delay(40);
      }
      Serial.println("volume up 0x490");
    }
    else //down
    {
      for (int i = 0; i < 3; i++)
      {
        irsend.sendSony(0xC90, 12);
        delay(40);
      }
      Serial.println("volume down 0xC90");
    }
    break;
  }
}
void setColor(int red, int green, int blue)
{
  analogWrite(ledR, red);
  analogWrite(ledG, green);
  analogWrite(ledB, blue);
}

long doPing()
{
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  return pulseIn(echoPin, HIGH);
}

long microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

