//
// Menu
//
#ifndef Draw_h
#define Draw_h
#include <AsyncDelay.h>



AsyncDelay refreshRate;
AsyncDelay stopDisplay;

const uint8_t timeRefresh = 50;
const uint16_t timeDisplay = 250000;

//
// Joystick pins
#define pinBtnNext 14
#define pinBtnBack 15
#define pinBtnOk 16
//
// Debounce time for the joystick
#define BtnDebounceTime 50
#define BtnHoldTime 2000

//
// Define custom characters
//
// An empty bar
const byte charEmptyBar[] = { B11111, B10001, B10001, B10001, B10001, B10001, B11111, B00000 };

class Draw {
private:
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
    byte state = digitalRead(pinBtn);

    if (!state) {
      debounceTime = millis();
      lastBtnPress = pinBtn;
    }

    if (!state && lastBtnPress == pinBtn && millis() - debounceTime > BtnDebounceTime)
      return true;

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


    if (!state && lastBtnPress == pinBtn && millis() - debounceTime > BtnDebounceTime) {
      if (millis() - debounceTime > BtnHoldTime) {
        //
        // Notify the user and mark as hold state
        this->isHold = true;
        this->playTick();
      }
    } else if (lastBtnPress == pinBtn && millis() - debounceTime > BtnDebounceTime) {

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


  void playTick() {
  }
  /**
    * Handles user general input
    */
  void input() {

    //
    // Present enter
    if (this->onClick(pinBtnOk) && !this->isEdit) {

      //
      // Activate menu or enter
      if (!this->displayOn) {
        this->displayOn = true;
      } else {
        this->enter = true;
      }

      stopDisplay.restart();
    }

    if (!this->displayOn || this->isEdit)
      return;

    //
    // Pressed next
    if (this->onClick(pinBtnNext)) {
      this->cursor++;
      stopDisplay.restart();
    }
    //
    // Pressed back
    if (this->onClick(pinBtnBack)) {
      this->cursor--;
      stopDisplay.restart();
    }
  }

  //
  // Pump edit
  void edit(Data data) {
    this->isDraw = true;
    if (!this->isEdit) return;
    //
    // Pressed next
    if (this->onClick(pinBtnNext)) {
      data.next();
      stopDisplay.restart();
    }
    //
    // Pressed back
    if (this->onClick(pinBtnBack)) {
      data.back();
      stopDisplay.restart();
    }

    HoldState state = this->onHold(pinBtnOk);
    if (Draw::Hold == state) {
      this->isEdit = !this->isEdit;
      data.save();
    }

    if (Draw::Tick == state) {
      this->isEdit = !this->isEdit;
      // exit?
    }
  }
  //
  // Controls the display suspend
  void suspendDisplay() {
    //
    // Turn off the display
    if (stopDisplay.isExpired()) {
      this->displayOn = false;
      this->isEdit = false;  // Close the edit menu
    }

    if (!this->displayOn) {
      lcd.noDisplay();
      //lcd.setBacklight(BACKLIGHT_OFF);
    } else {
      lcd.display();
      //lcd.setBacklight(BACKLIGHT_ON);
    }

    digitalWrite(pinBacklight, this->displayOn);
  }




public:

  Draw() {
  }

  //
  // Setup the menu
  void begin() {

    //
    // Setup the display type
    lcd.begin(16, 2);

    // lcd.createChar(0, charEmptyBar);

    refreshRate.start(timeRefresh, AsyncDelay::MILLIS);
    stopDisplay.start(timeDisplay, AsyncDelay::MILLIS);

    //
    // Print a Welcome message to the LCD.
    lcd.setCursor(0, 0);
    lcd.print(F("Welcome to"));
    lcd.setCursor(0, 1);
    lcd.print(F("the Water system"));

    //
    // Define simple joystick pins
    pinMode(pinBtnNext, INPUT_PULLUP);
    pinMode(pinBtnBack, INPUT_PULLUP);
    pinMode(pinBtnOk, INPUT_PULLUP);

    pinMode(pinBacklight, OUTPUT);
  }

  //
  // Drawse the menu and handles the inputs
  void draw(Menu* mn) {

    this->input();

    if (refreshRate.isExpired()) {
      refreshRate.repeat();
      this->isDraw = false;
    }

    this->suspendDisplay();
  }

  uint8_t getCursor() {
    return this->cursor;
  }

  void drawLevel(byte level) {

    if (level > 1)
      for (byte i = 0; i <= level; i++) {
        lcd.print((char)0xF);
      }

    lcd.blink();
  }
};

#endif
