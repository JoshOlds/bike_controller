#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);


int irPin = 4;                      // Input pin for IR sensor
int ledPin = 13;                    // Output LED pin

bool lastSensorReading = false;     //Store the last reading from the IR sensor
bool sensorReading = false;         //Current reading from IR sensor
unsigned long lastMicros = 0;       //Stores the last microsecond timestamp
unsigned long lastMillis = 0;
unsigned long elapsedMicros = 0;    //Stores the calculated elapsed microseconds
unsigned long ticksPerMinute = 0;   //Stores calculated ticks per minute NOT USED YET
int threshCount = 0;                //Don't change. Initializes global variable for threshCounter
float lastMagRead = 0.0f;

// USER DEFINED VARIABLES
unsigned long minSpeed = 200000;     // Minimum speed (max microseconds between ticks) to read. 
unsigned long microsBuffer[10];     //Buffer of the last XX readings. Used for averaging
int bufferLength = 10;              //Length of reading buffer. Changing this should dynamically work, but make sure to CHANGE BUFFER ARRAY SIZE above. Bigger buffer = smoother data, but longer calc times and more RAM usage
float threshold = 0.75f;            //Percentage required for addToMicrosBuffer() to accept data. Used to toss out bad readings due to bad paint job!
unsigned long dataRate = 20;        // Data rate (in milliseconds)



void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(irPin, INPUT);
  
  initializeMicrosBuffer();         //Fill buffer with zeros
  lastMicros = micros();            //Grab the microseconds since Arduino boot. This rolls over after ~70 minutes. could cause issues on long playtimes
  lastMillis = millis();

  Serial.begin(115200);             //Fast Serial Data. Ensure your console matches this rate!

   /* Enable auto-gain */
  mag.enableAutoRange(true);

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

  if(millis() - lastMillis >= dataRate){
    if(isMicrosBufferReady()){
      sendToSerial();
    }
    lastMillis = millis();
  }

  if(micros() - lastMicros >= minSpeed){
    addToMicrosBuffer(minSpeed, threshold);
    lastMicros = micros();
  }
}

void sendToSerial(){

  sensors_event_t event;
  mag.getEvent(&event);
  float magEvent = 0.0f;
  if(event.magnetic.x == 0.0f){
    magEvent = lastMagRead;
  }
  else{
    magEvent = event.magnetic.x;
    lastMagRead = magEvent;
  }

  Serial.print(getAverageMicrosBuffer());
  Serial.print(",");
  Serial.print(magEvent);
  Serial.println(); 
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


