#ifndef Menu_h
#define Menu_h

#include "Glob.h"



class Menu {
private:
  Data* mode;
  Data* tank1;
  Data* tank2;

  Rule* rl;

  /**
    * Draw level from 0 to 10 bars
    * @param level The level of the tank, from 100 /empty/ and 20 /full/
    * @param min An accured min value /100/
    */
  void
  drawLevel(byte level, byte min) {


    uint8_t bars = map(level, min, LevelSensorBothMax, 1, 10);
    // uint8_t bars = map(level, 100, 23, 1, 10);
    if (level > 0)
      for (byte i = 0; i <= level; i++) {
        lcd.write((uint8_t)0);
      }
  }

  /**
    * Display home screen
    */
  void home(DrawInterface* dr) {
    lcd.setCursor(0, 0);
    lcd.print(F("Tank1 "));
    int level1 = rl->getWellLevel();
    if (!level1)
      lcd.print(F("-?-"));
    else
      drawLevel(level1, LevelSensorWellMin);

    lcd.setCursor(0, 1);
    lcd.print(F("Tank2 "));
    int level2 = rl->getMainLevel();
    if (!level2)
      lcd.print(F("-?-"));
    else
      drawLevel(level2, LevelSensorMainMin);
  }

  void menuMode(DrawInterface* dr) {
    dr->edit(this->mode);
    lcd.setCursor(0, 0);
    lcd.print(F("Pump "));
    lcd.setCursor(0, 1);
    lcd.print(F("Mode: "));
    lcd.print(this->mode->getName());
    dbgLn(this->mode->getName());
  }


  void menuTank1(DrawInterface* dr) {
    dr->edit(this->tank1);
    lcd.setCursor(0, 0);
    lcd.print(F("Tank 1"));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->tank1->getName());
  }

  void menuTank2(DrawInterface* dr) {
    dr->edit(this->tank2);

    lcd.setCursor(0, 0);
    lcd.print(F("Tank 2"));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->tank2->getName());
  }


  void pumpWell(DrawInterface* dr) {

    dr->pump(&ctrlWell, &ctrlMain);

    lcd.setCursor(0, 0);
    lcd.print(F("Compressor: "));

    lcd.setCursor(0, 1);
    lcd.print(F("         >> "));
    if (ctrlWell.isOn())
      lcd.print(F("ON"));
    else
      lcd.print(F("OFF"));

    lcd.blink();
  }

  void pumpMain(DrawInterface* dr) {
    dr->pump(&ctrlMain, &ctrlWell);

    lcd.setCursor(0, 0);
    lcd.print(F("Pump to up: "));

    lcd.setCursor(0, 1);
    lcd.print(F("         >> "));
    if (ctrlMain.isOn())
      lcd.print(F("ON"));
    else
      lcd.print(F("OFF"));

    lcd.blink();
  }



public:
  //
  // Construct menu
  Menu(Rule* rl, Data* tk1, Data* tk2, Data* md)
    : rl(rl), tank1(tk1), tank2(tk2), mode(md) {
  }

  void begin() {
    //
    // Setup the display type
    lcd.begin(16, 2);

    byte charBarLevel[8] = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B00000 };

    lcd.createChar(0, charBarLevel);


    //
    // Print a Welcome message to the LCD.
    lcd.setCursor(0, 0);
    lcd.print(F("Automated"));
    lcd.setCursor(0, 1);
    lcd.print(F("  Water system "));
    lcd.blink();
    delay(1500);
  }
  //
  //Draw menu
  void draw(DrawInterface* dr) {

    if (!dr->isEditing()) lcd.noBlink();
    else lcd.blink();


    switch (dr->getCursor()) {

      case 0:
      default:
        this->home(dr);
        dr->resetCursor();

        break;

      case 1: return this->menuMode(dr);
      case 2: return this->menuTank1(dr);
      case 3: return this->menuTank2(dr);

      case 5: return this->pumpWell(dr);
      case 6: return this->pumpMain(dr);
    }
  }
};

#endif