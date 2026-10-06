#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

// Dwustronna komunikacja USB-UART. Ten szkic nie steruje silnikami.
// Odbior: CMD,numer,v_m_s,omega_rad_s\n
// Nadawanie: STATE,numer,v_zadane,omega_zadane,stan\r\n
// Pomiary enkoderow i pradu dolaczymy pozniej.
const uint32_t REPORT_INTERVAL_MS = 50;
const uint32_t COMMAND_TIMEOUT_MS = 600;
// Przykladowe limity komunikacji, do dopasowania do gotowego napedu.
const float MAX_LINEAR_SPEED = 1.0f;
const float MAX_ANGULAR_SPEED = 3.0f;

char receivedFrame[64];
uint8_t receivedLength = 0;
bool discardFrame = false;

uint32_t commandNumber = 0;
float linearSpeed = 0.0f;
float angularSpeed = 0.0f;
bool hasCommand = false;
bool timedOut = false;
uint32_t lastCommand = 0;
uint32_t lastReport = 0;

void initMotorControler();
void readSerialPort();
void processSerialFrame();
void checkCommandTimeout();
void sendSerialMessage();

void setup()
{
  initMotorControler();
}

void loop()
{
  readSerialPort();
  checkCommandTimeout();
  sendSerialMessage();
}

void initMotorControler()
{
  Serial.begin(115200);
  Serial.println(F("READY"));
  lastReport = millis();
}

void readSerialPort()
{
  // Budzet 32 znakow na wywolanie: inne funkcje tez moga dzialac.
  for (uint8_t n = 0; n < 32 && Serial.available() > 0; n++) {
    char receivedChar = Serial.read();

    if (receivedChar == '\r') {
      continue;
    }

    if (receivedChar == '\n') {
      if (discardFrame) {
        Serial.println(F("ERR,INVALID_FRAME"));
      }
      else if (receivedLength > 0) {
        receivedFrame[receivedLength] = '\0';
        processSerialFrame();
      }
      receivedLength = 0;
      discardFrame = false;
    }
    else if (!discardFrame) {
      // Tylko drukowalne ASCII; jedno miejsce zostaje na '\0'.
      if (receivedChar < 32 || receivedChar > 126 ||
          receivedLength >= sizeof(receivedFrame) - 1) {
        discardFrame = true;
      }
      else {
        receivedFrame[receivedLength++] = receivedChar;
      }
    }
  }
}

void processSerialFrame()
{
  if (strncmp(receivedFrame, "CMD,", 4) != 0) {
    Serial.println(F("ERR,UNKNOWN_COMMAND"));
    return;
  }

  char* start = receivedFrame + 4;
  char* end;
  if (*start < '0' || *start > '9') {
    Serial.println(F("ERR,COMMAND_NUMBER"));
    return;
  }

  errno = 0;
  unsigned long newNumber = strtoul(start, &end, 10);
  if (errno == ERANGE || newNumber > UINT32_MAX || *end != ',') {
    Serial.println(F("ERR,COMMAND_NUMBER"));
    return;
  }

  // Najpierw zmienne tymczasowe: zla ramka nie zmienia zadanych predkosci.
  start = end + 1;
  errno = 0;
  double newLinearSpeed = strtod(start, &end);
  if (end == start || errno == ERANGE || *end != ',' ||
      !isfinite(newLinearSpeed) || fabs(newLinearSpeed) > MAX_LINEAR_SPEED) {
    Serial.println(F("ERR,LINEAR_SPEED"));
    return;
  }

  start = end + 1;
  errno = 0;
  double newAngularSpeed = strtod(start, &end);
  if (end == start || errno == ERANGE || *end != '\0' ||
      !isfinite(newAngularSpeed) || fabs(newAngularSpeed) > MAX_ANGULAR_SPEED) {
    Serial.println(F("ERR,ANGULAR_SPEED"));
    return;
  }

  // Dopiero cala poprawna komenda aktualizuje wartosci i timeout.
  commandNumber = uint32_t(newNumber);
  linearSpeed = float(newLinearSpeed);
  angularSpeed = float(newAngularSpeed);
  hasCommand = true;
  timedOut = false;
  lastCommand = millis();
  // Ta sama ramka podtrzymuje timeout. Nie resetujemy tu regulatora.
}

void checkCommandTimeout()
{
  const uint32_t now = millis();
  if (hasCommand && !timedOut &&
      uint32_t(now - lastCommand) >= COMMAND_TIMEOUT_MS) {
    linearSpeed = 0.0f;
    angularSpeed = 0.0f;
    timedOut = true;
    // Po dodaniu napedow w tym miejscu trzeba rowniez zatrzymac wyjscia.
  }
}

void sendSerialMessage()
{
  const uint32_t now = millis();
  if (uint32_t(now - lastReport) < REPORT_INTERVAL_MS) {
    return;
  }
  // Raport przy powyzszych limitach miesci sie w 48 bajtach.
  // Zajety bufor: ponowimy probe w nastepnym loop(), bez czekania tutaj.
  if (Serial.availableForWrite() < 48) {
    return;
  }

  lastReport = now;
  Serial.print(F("STATE,"));
  Serial.print((unsigned long)commandNumber);
  Serial.print(',');
  Serial.print(linearSpeed, 4);
  Serial.print(',');
  Serial.print(angularSpeed, 4);
  Serial.print(',');

  if (!hasCommand) {
    Serial.println(F("WAITING"));
  }
  else if (timedOut) {
    Serial.println(F("TIMEOUT"));
  }
  else {
    Serial.println(F("OK"));
  }
}
