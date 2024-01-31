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
  virtual void edit(Data d) = 0;
  virtual void drawLevel(byte l) = 0;
};



//
// LCD display setup
#define pinBacklight 23
const uint8_t pinRs = 22, pinEn = 24, pinD4 = 25, pinD5 = 26, pinD6 = 27, pinD7 = 28;
LiquidCrystal lcd(pinRs, pinEn, pinD4, pinD5, pinD6, pinD7);

#include "Time.h"
#include "Util.h"
#include "Data.h"
#include "Rule.h"
#include "Menu.h"
#include "Draw.h"
#include "Cmd.h"

#endif