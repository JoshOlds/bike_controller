#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Servo.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);


int irPin = 4;                      // Input pin for IR sensor
int ledPin = 13;                    // Output LED pin
int servoPin = 6;                   // Servo output pin. Must be PWM pin deonoted with ~

bool lastSensorReading = false;     //Store the last reading from the IR sensor
bool sensorReading = false;         //Current reading from IR sensor
unsigned long lastMicros = 0;       //Stores the last microsecond timestamp
unsigned long lastMillis = 0;
unsigned long elapsedMicros = 0;    //Stores the calculated elapsed microseconds
unsigned long ticksPerMinute = 0;   //Stores calculated ticks per minute NOT USED YET
int threshCount = 0;                //Don't change. Initializes global variable for threshCounter
float lastMagRead = 0.0f;
float Pi = 3.14159;
Servo fanOutput;                    //Fan output is using the servo library for PWM

// USER DEFINED VARIABLES
unsigned long minSpeed = 200000;     // Minimum speed (max microseconds between ticks) to read. 
unsigned long microsBuffer[10];     //Buffer of the last XX readings. Used for averaging
int bufferLength = 10;              //Length of reading buffer. Changing this should dynamically work, but make sure to CHANGE BUFFER ARRAY SIZE above. Bigger buffer = smoother data, but longer calc times and more RAM usage
float threshold = 0.75f;            //Percentage required for addToMicrosBuffer() to accept data. Used to toss out bad readings due to bad paint job!
unsigned long dataRate = 20;        // Data rate (in milliseconds) -- this is output rate over serial line
int fanInitSpeed = 1000;
int fanMinSpeed = 1200;             // Fan minimum rate. Needed for initialization. PWM range is from 1000 - 2000
int fanMaxSpeed = 1800;             // Max fan speed. Deliberately low to not overstress fans and ESCs



void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(irPin, INPUT);
  
  initializeMicrosBuffer();         //Fill buffer with zeros
  lastMicros = micros();            //Grab the microseconds since Arduino boot. This rolls over after ~70 minutes. could cause issues on long playtimes
  lastMillis = millis();

  Serial.begin(115200);             //Fast Serial Data. Ensure your console matches this rate!

   /* Enable auto-gain */
  mag.enableAutoRange(true);

  fanOutput.attach(servoPin);        //Attach servo (fan) to pin
  fanOutput.write(fanInitSpeed);      //Initialize ESCs

    /* Initialise the sensor */
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
}

void loop() {                       //Main program loop

  
  
   sensorReading = digitalRead(irPin); //IR sensor reading
   
   if(sensorReading != lastSensorReading){
    elapsedMicros = micros() - lastMicros;        //Time difference between current time {micros()} and previous reading (lastMicros)
    if(sensorReading == HIGH && lastSensorReading == LOW){
      addToMicrosBuffer(elapsedMicros, threshold); //Add the elapsed time to buffer (if threshold passes)
    }
    lastMicros = micros();                        //Reset the previous reading time
    lastSensorReading = sensorReading;            //Reset the previous reading value
   }

  if(millis() - lastMillis >= dataRate){          //If it is time to send data down serial
    if(isMicrosBufferReady()){
      sendToSerial();
    }
    lastMillis = millis();                        //Reset last time 
  }

  if(micros() - lastMicros >= minSpeed){          //If sensor has not seen a change in the timeout duration
    addToMicrosBuffer(minSpeed, threshold);       // Add minspeed to buffer
    lastMicros = micros();
  }

  updateFans();                                   //Updates fan outputs. Comment out if no fans...
}

void updateFans(){
  float incomingFloat = 2.0f;
  if(Serial.available() > 0){ // If there are bytes on the serial line
    incomingFloat = Serial.parseFloat();//Grabs first float number on line. Breaks on first non-float character
    while(Serial.available()>0)
    {
      Serial.read(); // Flush remaining data
    }
  }
  else{return;} //Short circuit if none found
  
  if(incomingFloat >= 0.0f && incomingFloat <= 1.0f){
    if(incomingFloat == 0.0f){
      fanOutput.write(fanInitSpeed);
      return;
    }
    incomingFloat = (incomingFloat * (fanMaxSpeed - fanMinSpeed)) + fanMinSpeed; //Convert input of 0.0f to 1.0f to fan input range
    fanOutput.write(incomingFloat);
  }
}

void sendToSerial(){ //Send data readings down serial line

  sensors_event_t event; //Grab Adafruit sensor event
  mag.getEvent(&event); //Grab adafruit magnetometer event
  
  float magEvent = 0.0f;
  float heading = (atan2(event.magnetic.y,event.magnetic.x));// * 180) / Pi;
//  if (heading < 0) //Normalize
//  {
//    heading = 360 + heading;
//  }
  
  if(heading == 0.0f){ //If reading failed. We seem to have random 0.0 readings, and this filters those out
    magEvent = lastMagRead;
  }
  else{
    magEvent = heading;
    lastMagRead = magEvent;
  }

  Serial.print(getAverageMicrosBuffer()); //Gets the average speed from buffer
  Serial.print(","); //Comma seperated
  Serial.print(magEvent); //Mag heading
  Serial.println();  //Endline
}

void initializeMicrosBuffer(){  //Fill the buffer with zeros
  for(int i = 0; i < bufferLength; i++){ 
    microsBuffer[i] = 0;
  }
}

void addToMicrosBuffer(unsigned long value, float thresh){  //Add reading to buffer, if threshold passes. Max of 3 bad threshold readings before writing anyway.
//  if(microsBuffer[bufferLength - 1] != 0 && threshCount <= 3){
//    if(value > microsBuffer[bufferLength - 1] * (1.0f + thresh) || value < microsBuffer[bufferLength - 1] * (1.0f - (1.0f - thresh))){ //Thresholding to offset for crumby paint job!!
//      threshCount++; //Increment threshcount for override
//      return; 
//   }
//  }

  for(int i = 0; i < bufferLength - 1; i++){ //Shift all elements to the left
    microsBuffer[i] = microsBuffer[i+1];
  }
  microsBuffer[bufferLength-1] = value; //Add value to end of buffer
  threshCount = 0; //Reset threshold counter
}

unsigned long getAverageMicrosBuffer(){ //Returns the average of the buffer readings
  unsigned long average = 0;
  for(int i = 0; i < bufferLength; i++){
    average += microsBuffer[i];
  }
  average = average / bufferLength;
  if(average > minSpeed){ //Minimum speed override, set in global variables above.
    average = minSpeed;
  }
  return average;
}

bool isMicrosBufferReady(){ //Is the buffer ready? Cannot have any zeros in it!
  bool ready = true;
  for(int i = 0; i < bufferLength; i++){
    if(microsBuffer[i] == 0){
      ready = false;
    }
  }
  return ready;
}


