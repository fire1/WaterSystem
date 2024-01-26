


#ifndef Menu_h
#define Menu_h


class Menu {
private:

  Data& mode;
  Data& tank1;
  Data& tank2;




  void home() {
    lcd.setCursor(0, 0);
    lcd.print(F("Tank1 "));

    lcd.setCursor(0, 1);
    lcd.print(F("Tank2 "));
  }

  void menuMode(DrawInterface* dr) {
    dr->edit(this->mode);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Pump "));
    lcd.setCursor(0, 1);
    lcd.print(F("Mode: "));
    lcd.print(this->mode.getName());
  }


  void menuTank1(DrawInterface* dr) {
    dr->edit(this->tank1);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Tank 1"));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->tank1.getName());
  }

  void menuTank2(DrawInterface* dr) {
    dr->edit(this->tank2);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Tank 2"));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->tank2.getName());
  }



public:
  //
  // Construct menu
  Menu(Data& tk1, Data& tk2, Data& md)
    : tank1(tk1), tank2(tk2), mode(md) {
  }
  //
  //Draw menu
  void draw(DrawInterface* dr) {
    switch (dr->getCursor()) {

      case 0:
      default:
        this->home();
        break;

      case 1: return this->menuMode(dr);
      case 2: return this->menuTank1(dr);
      case 3: return this->menuTank2(dr);
    }
  }
};

#endif