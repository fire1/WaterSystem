#ifndef MenuInterface_h
#define MenuInterface_h

//
// Pump controll pins
#define pinWellPump A8
#define pinMainPump A9

class Draw;
class Menu;
class Data;
class Pump;



class DrawInterface {
public:

  // Pure virtual functions - These functions must be implemented by derived classes
  virtual uint8_t getCursor() = 0;
  virtual void edit(Data* d) = 0;
  virtual void pump(Pump* p) = 0;
  virtual void resetCursor();
  virtual bool isEditing() = 0;
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
// Define Ultrasonic mesurment pins
#define pinWellEcho 18
#define pinWellSend 19

// #define LevelsRefreshTime 60000
#define LevelsRefreshTime 5000

#define pinMainPower 8  // Turn on (GND) power for slave
#define pinMainRx 10    // Recive data pin from slave
//
// Defines how meny time to read sensors
//  before defining tank state.
#define LevelSensorReads 5

//
// This value defines safe level point for max u-s sensor
//  reads. Shoud be common for both sensors.
#define LevelSensorBothMax 23
#define LevelSensorWellMin 95
#define LevelSensorMainMin 95


#define SuspendDisplayTime 120000  // 5min
//
// Debounce time for the joystick
#define BtnDebounceTime 50
#define BtnHoldTime 2000


#include "Time.h"
#include "Pump.h"

extern Pump ctrlWell(pinWellPump, pinBtnWell, pinLedWell);
extern Pump ctrlMain(pinMainPump, pinBtnMain, pinLedMain);

#include "Tone.h"
#include "Util.h"
#include "Data.h"
#include "Rule.h"
#include "Menu.h"
#include "Draw.h"
#include "Cmd.h"






#endif
