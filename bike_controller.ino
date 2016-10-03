

int irPin = 4;                      // Input pin for IR sensor
int ledPin = 13;                    // Output LED pin

bool lastSensorReading = false;     //Store the last reading from the IR sensor
bool sensorReading = false;         //Current reading from IR sensor
unsigned long lastMicros = 0;       //Stores the last microsecond timestamp
unsigned long elapsedMicros = 0;    //Stores the calculated elapsed microseconds
unsigned long ticksPerMinute = 0;   //Stores calculated ticks per minute NOT USED YET
int threshCount = 0;                //Don't change. Initializes global variable for threshCounter

// USER DEFINED VARIABLES
unsigned long minSpeed = 50000;     // Minimum speed (max microseconds between ticks) to read. 
unsigned long microsBuffer[40];     //Buffer of the last 40 readings. Used for averaging
int bufferLength = 40;              //Length of reading buffer. Changing this should dynamically work, but make sure to CHANGE BUFFER ARRAY SIZE above. Bigger buffer = smoother data, but longer calc times and more RAM usage
float threshold = 0.75f;            //Percentage required for addToMicrosBuffer() to accept data. Used to toss out bad readings due to bad paint job!


void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(irPin, INPUT);
  
  initializeMicrosBuffer();         //Fill buffer with zeros
  lastMicros = micros();            //Grab the microseconds since Arduino boot. This rolls over after ~70 minutes. could cause issues on long playtimes

  Serial.begin(115200);             //Fast Serial Data. Ensure your console matches this rate!
}

void loop() {                       //Main program loop

   sensorReading = digitalRead(irPin); //IR sensor reading
   
   if(sensorReading != lastSensorReading){
    elapsedMicros = micros() - lastMicros;        //Time difference between current time {micros()} and previous reading (lastMicros)
    addToMicrosBuffer(elapsedMicros, threshold);  //Add the elapsed time to buffer (if threshold passes)
    lastMicros = micros();                        //Reset the previous reading time
    lastSensorReading = sensorReading;            //Reset the previous reading value
    
    if(isMicrosBufferReady()){                    //If the buffer is ready (not full of zeros), then write to serial port
      Serial.print(getAverageMicrosBuffer());
      Serial.print("\t");
      Serial.print(50000);
      Serial.print("\t");
      Serial.print(0);
      Serial.println();
    }
   }   
}

void initializeMicrosBuffer(){  //Fill the buffer with zeros
  for(int i = 0; i < bufferLength; i++){ 
    microsBuffer[i] = 0;
  }
}

void addToMicrosBuffer(unsigned long value, float thresh){  //Add reading to buffer, if threshold passes. Max of 3 bad threshold readings before writing anyway.
  if(microsBuffer[bufferLength - 1] != 0 && threshCount <= 3){
    if(value > microsBuffer[bufferLength - 1] * (1.0f + thresh) || value < microsBuffer[bufferLength - 1] * (1.0f - (1.0f - thresh))){ //Thresholding to offset for crumby paint job!!
      threshCount++; //Increment threshcount for override
      return; 
   }
  }

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


