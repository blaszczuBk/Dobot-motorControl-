#include <Arduino.h>
#include <string.h>
#include <errno.h>

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