


#ifndef Menu_h
#define Menu_h


class Menu {
private:

  Data* mode;
  Data* tank1;
  Data* tank2;

  Rule* rl;

  /**
    * Draw level from 0 to 10 bars
    */
  void drawLevel(byte level) {

    if (level > 1)
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
    int level1 = rl->getWellBars();
    if (!level1)
      lcd.print(F("-?-"));
    else
      drawLevel(level1);

    lcd.setCursor(0, 1);
    lcd.print(F("Tank2 "));
    int level2 = rl->getRiseBars();
    if (!level2)
      lcd.print(F("-?-"));
    else
      drawLevel(level2);
  }

  void menuMode(DrawInterface* dr) {
    dr->edit(this->mode);
    lcd.setCursor(0, 0);
    lcd.print(F("Pump "));
    lcd.setCursor(0, 1);
    lcd.print(F("Mode: "));
    lcd.print(this->mode->getName());
    dbgLn(this->mode->getName());
    lcd.blink();
  }


  void menuTank1(DrawInterface* dr) {
    dr->edit(this->tank1);
    lcd.setCursor(0, 0);
    lcd.print(F("Tank 1"));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->tank1->getName());
    lcd.blink();
  }

  void menuTank2(DrawInterface* dr) {
    dr->edit(this->tank2);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Tank 2"));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->tank2->getName());
    lcd.blink();
  }



public:
  //
  // Construct menu
  Menu(Data* tk1, Data* tk2, Data* md)
    : tank1(tk1), tank2(tk2), mode(md) {
  }
  //
  //Draw menu
  void draw(DrawInterface* dr) {
    switch (dr->getCursor()) {

      case 0:
      default:
        this->home(dr);
        dr->resetCursor();
        break;

      case 1: return this->menuMode(dr);
      case 2: return this->menuTank1(dr);
      case 3: return this->menuTank2(dr);
    }
  }

  void pass(Rule* rl) {
    this->rl = rl;
  }
};

#endif