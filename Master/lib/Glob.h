#ifndef MenuInterface_h
#define MenuInterface_h


//
// Defines global levels of water containers
static byte containerLevels[2] = { 0, 0 };

class Draw;
class Menu;
class Data;

class DrawInterface {
public:
  // Pure virtual functions - These functions must be implemented by derived classes
  virtual uint8_t getCursor() = 0;
  virtual void edit(Data* d) = 0;
  virtual void resetCursor();
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
#define pinLedRise 32

//
// Instant pump buttons
#define pinBtnWell 33
#define pinBtnRise 34


//
// Panel navigation pins
#define pinBtnBack 35
#define pinBtnOk 36
#define pinBtnNext 37

//
// Define Ultrasonic mesurment pins
#define pinWellEcho 18
#define pinWellSend 17

//
// Debounce time for the joystick
#define BtnDebounceTime 100
#define BtnHoldTime 2000


#include "Time.h"


#include "Tone.h"
#include "Util.h"
#include "Data.h"
#include "Rule.h"
#include "Menu.h"
#include "Draw.h"
#include "Cmd.h"






#endif
