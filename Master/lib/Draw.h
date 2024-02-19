//
// Menu
//
#ifndef Draw_h
#define Draw_h
#include <AsyncDelay.h>
#include <LiquidCrystal.h>



// AsyncDelay refreshRate;
AsyncDelay stopDisplay;





class Draw : public DrawInterface {
private:
  Buzz* buzz;
  Read* read;

  enum HoldState {
    None = 0,
    Tick = 1,
    Hold = 2,
  };

  unsigned long debounceTime = 0;
  uint8_t lastBtnPress = 0;
  uint8_t cursor = 0;
  bool enter = false;
  bool displayOn = true;
  bool isEdit = false;
  bool isHold = false;
  bool isDraw = false;

  /**
  * Method to capture button presses
  */
  bool onClick(uint8_t pinBtn) {

    if (!lastBtnPress && !digitalRead(pinBtn) && millis() + debounceTime > BtnDebounceTime) {
      lastBtnPress = pinBtn;
      debounceTime = 0;
      stopDisplay.restart();
      return true;
    }

    if (!digitalRead(pinBtn) && !lastBtnPress) {
      debounceTime = millis();
    }

    if (lastBtnPress == pinBtn && digitalRead(pinBtn))
      lastBtnPress = 0;

    return false;
  }
  /**
  * Method to hold button presses
  */
  HoldState onHold(uint8_t pinBtn) {

    byte state = digitalRead(pinBtn);

    if (state == LOW) {
      debounceTime = millis();
      lastBtnPress = pinBtn;
    }


    if (!lastBtnPress && digitalRead(pinBtn) && millis() + debounceTime > BtnDebounceTime) {
      if (millis() + debounceTime > BtnHoldTime) {
        //
        // Notify the user and mark as hold state
        this->isHold = true;
        this->playTick();
      }
    } else if (lastBtnPress == pinBtn && !digitalRead(pinBtn)) {

      if (this->isHold) {
        this->isHold = false;
        return Draw::Hold;
      }
      //
      // Return tick instead of hold
      return Draw::Tick;
    }
    return Draw::None;
  }


  void
  playTick() {
  }
  /**
    * Handles user general input
    */
  void input() {



    if (!this->displayOn) {
      if (this->onClick(pinBtnOk) || this->onClick(pinBtnBack) || this->onClick(pinBtnNext) || this->onClick(pinBtnWell) || this->onClick(pinBtnMain)) {
        this->weakUpDisplay();
        return;
      }
    }

    if (!this->displayOn || this->isEdit) {
      return;
    }

    //
    // Present enter
    if (this->onClick(pinBtnOk) && !this->isEdit) {
      dbgLn(F("BTN OK pressed"));
      if (this->cursor > 0) {
        buzz->enter();
        this->isEdit = true;
      }
    }


    //
    // Pressed next
    if (this->onClick(pinBtnNext)) {
      dbgLn(F("BTN Next pressed"));
      buzz->click();
      this->cursor++;
    }
    //
    // Pressed back
    if (this->onClick(pinBtnBack)) {
      dbgLn(F("BTN Back pressed"));
      buzz->click();
      this->cursor--;
    }

    if (this->onClick(pinBtnWell)) {
      dbgLn(F("BTN Well pressed"));
      buzz->mode();
      this->cursor = 5;
    }

    if (this->onClick(pinBtnMain)) {
      buzz->mode();
      dbgLn(F("BTN Main pressed"));
      this->cursor = 6;
    }
  }


  //
  // Pump edit
  void edit(Data* data) {
    this->isDraw = true;
    if (!this->isEdit) return;

    //
    // Pressed next
    if (this->onClick(pinBtnNext)) {
      dbgLn(F("Data next "));
      buzz->click();
      data->next();
    }
    //
    // Pressed back
    if (this->onClick(pinBtnBack)) {
      dbgLn(F("Data back "));
      buzz->click();
      data->back();
    }

    //HoldState state = this->onClick(pinBtnOk);
    if (this->onClick(pinBtnOk)) {
      this->isEdit = !this->isEdit;
      dbgLn(F("Data save "));
      buzz->save();
      data->save();
    }
    /*
    if (Draw::Tick == state) {
      this->isEdit = !this->isEdit;
      // exit?
    }
    */
  }
  //
  // Controls the display suspend
  void suspendDisplay() {
    //
    // Turn off the display
    if (stopDisplay.isExpired()) {

      //
      // When pumps are not working and display sleep
      if (!ctrlWell.isOn() && !ctrlMain.isOn())
        read->stopWorkRead();  // Stop fast read


      this->displayOn = false;
      this->isEdit = false;  // Close the edit menu
      this->cursor = 0;      // Reset the menu
    }

    if (!this->displayOn) {
      lcd.noDisplay();
    } else {
      lcd.display();
    }

    digitalWrite(pinBacklight, this->displayOn);
  }

  //
  // Weaks up the display
  void weakUpDisplay() {
    this->displayOn = true;
    this->cursor = 0;
    stopDisplay.restart();
    lcd.display();
    digitalWrite(pinBacklight, this->displayOn);
    read->startWorkRead();
  }




public:
  Draw(Read* rd, Buzz* tn)
    : read(rd), buzz(tn) {
  }

  bool isEditing() {
    return this->isEdit;
  }
  //
  // Toggle pump state on click
  void pump(Pump* pump, Pump* stop) {
    this->isEdit = true;

    if (this->onClick(pinBtnOk) || this->onClick(pinBtnNext) || this->onClick(pinBtnBack)) {
      this->isEdit = false;
      this->cursor = 0;
    }

    if (this->onClick(pump->getBtn())) {
      pump->toggle();
      if (stop->isOn())
        stop->setOn(false);

      dbg(F("Activated pump pin: "));
      dbg(pump->getPin());
      dbg(F(" / state: "));
      dbg(pump->isOn());
      dbgLn();
      this->cursor = 0;
      this->isEdit = false;
      buzz->save();
    }
  }
  //
  // Setup the menu
  void begin() {

    // refreshRate.start(timeRefresh, AsyncDelay::MILLIS);
    stopDisplay.start(SuspendDisplayTime, AsyncDelay::MILLIS);
    //
    // Define simple joystick pins
    pinMode(pinBtnNext, INPUT_PULLUP);
    pinMode(pinBtnBack, INPUT_PULLUP);
    pinMode(pinBtnOk, INPUT_PULLUP);

    //
    // Instant pump buttons
    pinMode(pinBtnWell, INPUT_PULLUP);
    pinMode(pinBtnMain, INPUT_PULLUP);



    //
    // Pin leds
    pinMode(pinLedWell, OUTPUT);
    pinMode(pinLedMain, OUTPUT);
    pinMode(pinLedBeat, OUTPUT);

    //
    // Display backlight
    pinMode(pinBacklight, OUTPUT);
    digitalWrite(pinBacklight, HIGH);

    digitalWrite(pinLedWell, LOW);
    digitalWrite(pinLedMain, LOW);
    digitalWrite(pinLedBeat, LOW);
    delay(200);

    digitalWrite(pinLedWell, HIGH);
    digitalWrite(pinLedMain, HIGH);
    digitalWrite(pinLedBeat, HIGH);
  }

  //
  // Drawse the menu and handles the inputs
  void menu(Menu* mn) {

    this->input();

    if (spanMd.isActive()) {
      lcd.clear();
      if (cursor == 0)
        read->startWorkRead();  // Start fast read

      this->isDraw = false;
      mn->draw(this);
    }

    this->suspendDisplay();
  }

  uint8_t getCursor() {
    return this->cursor;
  }

  void resetCursor() {
    this->cursor = 0;
  }
};

#endif
