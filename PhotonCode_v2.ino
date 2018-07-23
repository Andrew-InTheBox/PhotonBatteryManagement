// This #include statement was automatically added by the Particle IDE.
#include <application.h>
#include <neopixel.h> 

//Define neopixel related variables for LED
#define PIXEL_PIN D4
#define PIXEL_COUNT 12
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

int numberOfBoards;
int msb;
int lsb;
int cSense = A0; // current sensor use as ADC
int preCharge = D2; // precharge digital out
int mainRelay = D3; // main relay digital out
int manOver = D5; //switch to override error state
int myval = 0;
double current; //holds current measurement
double analogVoltage; //holds ADC for current measurement
int analogValue; //raw ADC value from current sensor
int extTemp1; //extTemp sensor from BMB board
int voltageArray[12] = {4000,4000,4000,4000,4000,4000,4000,4000,4000,4000,4000,4000}; //holds cell voltage values
unsigned long chrono = millis(); //counter for state machine activities
enum masterState {ON,ERROR1,ERRORRESP}; //state machine possible states
int measureDelay = 3000; //delay value for state machine
masterState myState;  //need to call 'enum' here again?


void setup() {
    pinMode(cSense, INPUT);
    pinMode(preCharge, OUTPUT);
    pinMode(mainRelay, OUTPUT);
    pinMode(manOver, INPUT_PULLUP);
    //pinMode(addLED, OUTPUT);
    Wire.begin(); // join i2c bus
    Serial.begin(9600);
    strip.begin(); //initialize LED 
    strip.show(); // Initialize all pixels to 'off'
    numberOfBoards = checkForBoards(); //Read how many boards are detected
    startUp(); //function below that precharge and sets initial state to 'ON'
    //delay(20000);
    updateValues();
    //Particle.function("Override",relayOverride); //function to allow web ovrride of error state
}



void stateMachine() {
  //while(FALSE){

  switch (myState) {
  case ON:
    {
      delay(1000);

      for (int i = 0; i < PIXEL_COUNT; i++) {
        strip.setPixelColor(i, 0, 255, 0);
        strip.show();
      }

      //LED to 'good' indication
      if (millis() - chrono >= measureDelay) {
        Serial.println("ON state");
        chrono = millis();
        updateValues();
        for (byte i = 0; i < 12; i++) {
          if (voltageArray[i] > 4100 | voltageArray[i] < 3300) {
            myState = ERROR1;
            break;
          } else {
            myState = ON;
          }
        }
        //write values to photon web function, or do within updateValues()
      }
      break;
    }
  case ERROR1:
    {
      shutDown();

      for (int i = 0; i < PIXEL_COUNT; i++) {
        strip.setPixelColor(i, 255, 0, 0);
        strip.show();
      }
      updateValues();
      Serial.println("ERROR1 state");
      myval = digitalRead(manOver);
      Serial.print("OverrideValue: ");
      Serial.println(myval);
      if (myval == HIGH) {
        myState = ERRORRESP;
      }
      //close relays
      //notify
      //LED to 'error' indication
      break;
    }
  case ERRORRESP:
    {
      //idle until manual switch closes to initiate override
      //then close relays
      //wait to allow charging etc then
      startUp();
      updateValues();
      for (int i = 0; i < PIXEL_COUNT; i++) {
        strip.setPixelColor(i, 255, 255, 0);
        strip.show();
      }
      
      Serial.println("ERROR-RESP state");
      myval = digitalRead(manOver);
      Serial.print("OverrideValue: ");
      Serial.println(myval);
      if (myval == LOW) {
        myState = ON;
      } else {
        myState = ERRORRESP;
      }

      break;

    }

  }
}


void loop() {
    stateMachine();
}

void updateValues() {
    for(byte board = 1; board<=numberOfBoards; board++) { //Cycle through each detected board
        Serial.print("Board ");
        Serial.print(board);
        Serial.println(":"); //Print out which board is being read from
        for(byte cell = 1; cell<13; cell++) { //Cycle through each cell
            Wire.beginTransmission(8); // transmit to device #8 (BMB default I2C address)
            Wire.write(board);          // talk to the first board
            Wire.write(cell);          // sends one byte
            Wire.endTransmission();    // stop transmitting
            Wire.requestFrom(8,2);     // Request 2 bytes from slave 8
            while(Wire.available()) {
                msb = Wire.read();  // first byte is most significant
                lsb = Wire.read();  // second byte is least significant
            }
            //int voltage = (msb<<8) | (lsb); //Voltage in mV, 12bit resoultion
            voltageArray[cell-1] = ((msb & 0xFF) | (lsb & 0xFF) << 8);
            Serial.print("Cell ");
            Serial.print(cell);
            Serial.print(": ");
            Serial.println(voltageArray[cell-1]); //Print the voltages of each cell on each board
            //Particle.publish("VoltReport: ", String(voltageArray[cell-1]));

        }
        analogValue = analogRead(cSense);
        analogVoltage = (analogValue * 0.000805861);
        analogVoltage = (analogVoltage - 1.65);
        current = (analogVoltage / .045);
        Serial.print("AnalogVoltageCurrentSensor: ");
        Serial.println(analogVoltage);
        Serial.print("Current: ");
        Serial.println(current);
        //Particle.publish("Current: ", String(current), PRIVATE); //publish current value
        Wire.beginTransmission(8); // transmit to device #8 (BMB default I2C address)
        Wire.write(board);          // talk to the first board
        Wire.write(17);          // sends one byte
        Wire.endTransmission();    // stop transmitting
        Wire.requestFrom(8,1);     // Request 1 byte from slave 8
        while(Wire.available()) {
            extTemp1 = Wire.read();  // get temp sensor 1 data
        }
        Serial.print("extTemp1: ");
        Serial.println(extTemp1);
        //Particle.publish("extTemp1: ", String(extTemp1), PRIVATE); //publish temp sensor value
    }
}

void startUp() {
    digitalWrite(preCharge, HIGH);
    delay(500);
    digitalWrite(mainRelay, HIGH);
    digitalWrite(preCharge, LOW);
    myState = ON;
}

void shutDown() {
    digitalWrite(preCharge, LOW);
    delay(100);
    digitalWrite(mainRelay, LOW);
}

byte checkForBoards() {
    byte boardNumber;
    Wire.beginTransmission(8); // transmit to device #8 (BMB default I2C address)
    Wire.write(0x7E);          // request number of boards connected
    Wire.endTransmission();    // send out write Command

    Wire.requestFrom(8,1);     // request 1 byte from device #8
    while(Wire.available()) {  // continue reading until buffer is empty
        boardNumber = Wire.read(); // read the information
    }

    return boardNumber;
}



