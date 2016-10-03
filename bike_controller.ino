

int irPin = 4;    // Input pin for IR sensor
int ledPin = 13;      // Output LED pin

bool lastSensorReading = false; 
bool sensorReading = false;
unsigned long lastMicros = 0;
unsigned long elapsedMicros = 0;
unsigned long ticksPerMinute = 0;
unsigned long microsBuffer[40];
unsigned long minSpeed = 50000;
int bufferLength = 40;
float threshold = 0.75f;
int threshCount = 0;

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);
  pinMode(irPin, INPUT);
  
  initializeMicrosBuffer();
  lastMicros = micros(); // Grab the microseconds since Arduino boot. This rolls over after ~70 minutes

  Serial.begin(115200); // Fast Serial Data
}

void loop() {

   sensorReading = digitalRead(irPin);
   
   if(sensorReading != lastSensorReading){
    elapsedMicros = micros() - lastMicros;
    addToMicrosBuffer(elapsedMicros, threshold);
    lastMicros = micros();
    lastSensorReading = sensorReading;
    if(isMicrosBufferReady()){
      
      Serial.print(getAverageMicrosBuffer());
      Serial.print("\t");
      Serial.print(50000);
      Serial.print("\t");
      Serial.print(0);
      Serial.println();
    }
   }   
}

void initializeMicrosBuffer(){
  for(int i = 0; i < bufferLength; i++){ //Shift all elements to the left
    microsBuffer[i] = 0;
  }
}

void addToMicrosBuffer(unsigned long value, float thresh){
  if(microsBuffer[bufferLength - 1] != 0 && threshCount <= 3){
    if(value > microsBuffer[bufferLength - 1] * (1.0f + thresh) || value < microsBuffer[bufferLength - 1] * (1.0f - (1.0f - thresh))){ //Thresholding to offset for crumby paint job!!
      threshCount++;
      //Serial.println("Thresholding!");
      return; 
   }
  }

  for(int i = 0; i < bufferLength - 1; i++){ //Shift all elements to the left
    microsBuffer[i] = microsBuffer[i+1];
  }
  microsBuffer[bufferLength-1] = value;
  threshCount = 0;
}

unsigned long getAverageMicrosBuffer(){
  unsigned long average = 0;
  for(int i = 0; i < bufferLength; i++){
    average += microsBuffer[i];
  }
  average = average / bufferLength;
  if(average > minSpeed){
    average = minSpeed;
  }
  return average;
}

bool isMicrosBufferReady(){
  bool ready = true;
  for(int i = 0; i < bufferLength; i++){
    if(microsBuffer[i] == 0){
      ready = false;
    }
  }
  return ready;
}


