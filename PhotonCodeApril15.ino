#include <application.h>

int numberOfBoards;
int msb;
int lsb;
int cSense = A0; // current sensor use as ADC
int preCharge = D2; // precharge digital out
int mainRelay = D3; // main relay digital out
int addLED = D4; // addressable LED, not currently used
double current;
double analogVoltage;
int analogValue;

void setup() {
  pinMode(cSense, INPUT);
  pinMode(preCharge, OUTPUT);
  pinMode(mainRelay, OUTPUT);
  Wire.begin(); // join i2c bus
  Serial.begin(9600);
  numberOfBoards = checkForBoards(); //Read how many boards are detected
}

void loop() {
  for(byte board = 1;board<=numberOfBoards; board++){ //Cycle through each detected board
    Serial.print("Board ");
    Serial.print(board);
    Serial.println(":"); //Print out which board is being read from
    for(byte cell = 1;cell<13;cell++){ //Cycle through each cell
      Wire.beginTransmission(8); // transmit to device #8 (BMB default I2C address)
      Wire.write(board);          // talk to the first board
      Wire.write(cell);          // sends one byte
      Wire.endTransmission();    // stop transmitting
      Wire.requestFrom(8,2);     // Request 2 bytes from slave 8
      while(Wire.available()){
        msb = Wire.read();  // first byte is most significant
        lsb = Wire.read();  // second byte is least significant
      }
      //int voltage = (msb<<8) | (lsb); //Voltage in mV, 12bit resoultion
      int voltage = ((msb & 0xFF) | (lsb & 0x0F) << 8);
      Serial.print("Cell ");
      Serial.print(cell);
      Serial.print(": ");
      Serial.println(voltage); //Print the voltages of each cell on each board
      Serial.print("lsb: ");
      Serial.println(lsb);
      Serial.print("msb: ");
      Serial.println(msb);
    }
    analogValue = analogRead(cSense);
    Serial.print("ADC value: ");
    Serial.println(analogValue); //sensor is 45mV/A
    analogVoltage = (analogValue * 0.000805861);
    analogVoltage = (analogVoltage - 1.65);
    current = (analogVoltage / 0.45);
    Serial.print("analogVoltage: ");
    Serial.println(analogVoltage);
    Serial.print("Current: ");
    Serial.println(current);
    //current = (((analogValue * 3.3)/4095) - 0.5) * 100;
  }
  delay(1000);
}



byte checkForBoards(){
  byte boardNumber;
  Wire.beginTransmission(8); // transmit to device #8 (BMB default I2C address)
  Wire.write(0x7E);          // request number of boards connected
  Wire.endTransmission();    // send out write Command

  Wire.requestFrom(8,1);     // request 1 byte from device #8
  while(Wire.available()){   // continue reading until buffer is empty
    boardNumber = Wire.read(); // read the information
  }

  return boardNumber;
}