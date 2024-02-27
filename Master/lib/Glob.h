#ifndef Global_h
#define Global_h


#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <AsyncDelay.h>
#include <Wire.h>


#define pinLed 13
#define pinTone 9
//
// Pump controll pins
#define pinWellPump A8
#define pinMainPump A9

//
// Baud rate for Slave
#define BaudSlaveRx 4800

class Draw;
class Menu;
class Data;
class Pump;
class Tone;


class DrawInterface {
public:

  // Pure virtual functions - These functions must be implemented by derived classes
  virtual uint8_t getCursor() = 0;
  virtual void edit(Data* d) = 0;
  virtual void pump(Pump* p, Pump* s) = 0;
  virtual void resetCursor();
  virtual bool isEditing() = 0;
  virtual bool isDisplayOn() = 0;
};



//
// LCD display setup
#define pinBacklight 29
const uint8_t pinRs = 22, pinEn = 23, pinD4 = 24, pinD5 = 25, pinD6 = 26, pinD7 = 27;
LiquidCrystal lcd(pinRs, pinEn, pinD4, pinD5, pinD6, pinD7);

//
// Panel led pins
#define pinLedBeat 30
#define pinLedWell 31
#define pinLedMain 32

//
// Instant pump buttons
#define pinBtnWell 33
#define pinBtnMain 34


//
// Panel navigation pins
#define pinBtnBack 35
#define pinBtnOk 36
#define pinBtnNext 37

//
//  Ultrasonic Standart read mesurment pinns,
//    it is not recommended sice uses too much time to measure
#ifdef WELL_MEASURE_DEFAULT
#define pinWellEcho 15  // Echo pin
#define pinWellSend 14  // Trigger pin
#endif
//
// For this mode a 45Kohm resistor is solder for R19
#ifdef WELL_MEASURE_UART_47K   // For UART Serial I'm using Serial3 port RX3 pin 15 and TX3 pin 14
#define startUartCommand 0x01  // All available commands to transmit are 0x01, 0x00, 0x55
#define verifyCorrection 1     // Some of the sensors sends +/-  1/2 values, in my case is +1 in order to work verification.
#endif



#define LevelRefreshTimeIdle 1800000  // 30min
// #define LevelRefreshTimeIdle 900000  // 15min
#define LevelRefreshTimeWork 5000

#define pinMainPower 8  // Turn on (GND) power for slave
#define pinMainRx 10    // Recive data pin from slave
//
// Defines how meny time to read sensors
//  before defining tank state.
#define LevelSensorReads 3

//
// This value defines safe level point for max u-s sensor
//  reads. Shoud be common for both sensors.
#define LevelSensorBothMax 21
#define LevelSensorWellMin 95
#define LevelSensorMainMin 100


#define SuspendDisplayTime 120000  // 2min
#define DisableSensorError 3
//
// Debounce time for the joystick
#define BtnDebounceTime 10
#define BtnHoldTime 2000




#include "Pump.h"
#include "Util.h"
#include "Data.h"
#include "Span.h"
#include "Buzz.h"

extern Pump ctrlWell(pinWellPump, pinBtnWell, pinLedWell);
extern Pump ctrlMain(pinMainPump, pinBtnMain, pinLedMain);

extern Span spanSm(199);     // Loop span at Small
extern Span spanMd(2101);    // Loop span Middle  /screen refresh/
extern Span spanLg(4802);    // Loop span Large  / warning messages/
extern Span spanMx(250003);  // Loop span at 60k loops

#include "Time.h"
#include "Read.h"
#include "Rule.h"
#include "Menu.h"
#include "Draw.h"
#include "Cmd.h"


#endif
