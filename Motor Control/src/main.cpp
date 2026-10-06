#include <Arduino.h>
#include <string.h>
#include <errno.h>

//Definition of pin to control the bts7968 [PWM_F - forward, PWM_R - revers]
const uint8_t EN_R[] = {2, 7}, EN_L[] = {4, 8};
const uint8_t PWM_F[] = {6, 9}, PWM_R[] = {5, 10};
uint8_t PWM_Limit = 100;
uint8_t RAMO_MS = 10;

//Current sensors pins
const uint8_t Current_Sensors[] = {A6, A7};

//Data for parsing the communication frames
char receivedFrame[64];
uint8_t receivedLength = 0;
bool discardFrame = false;

void initMotorControler();
void readSerialPort();
void processSerialFrame();

void setup() {
  initMotorControler();
}

void loop() { 
  readSerialPort();
}


void initMotorControler()
{
  Serial.begin(115200);
}

void readSerialPort()
{
  if (Serial.available() == 0) {return;  }

  char receivedChar = Serial.read();

  if (receivedChar == '\r') {return;}

  if (receivedChar == '\n') {
    if (!discardFrame && receivedLength > 0) {
      receivedFrame[receivedLength] = '\0';
      processSerialFrame();
    }

    receivedLength = 0;
    discardFrame = false;
  }else if (!discardFrame) {
    if (receivedLength < sizeof(receivedFrame) - 1) {
      receivedFrame[receivedLength] = receivedChar;
      receivedLength++;
    }
    else {
      discardFrame = true;
    }
  }
}

void processSerialFrame()
{
  if(strncmp(receivedFrame, "CMD,", 4)!=0)
  {
    return;
  }
}