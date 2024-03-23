
#ifndef Menu_h
#define Menu_h

#include "Glob.h"

class Menu {
private:
  Rule *rule;
  Read *read;
  Time *time;
  Heat *heat;
  Data *modeWell;
  Data *modeMain;
  char formatBuffer[4];

  //
  // At init state needs to be as same as Draw.h edit state.
  bool isEditLast = false;

  /**
      * Draw level from 0 to 10 bars
      * @param level The level of the tank, from 100 /empty/ and 20 /full/
      * @param min An accured min value /100/
      */
  void drawLevel(byte level, byte min) {

    uint8_t bars = map(level, min + 2, LevelSensorBothMax + 5, 1, 10);
    // uint8_t bars = map(level, 100, 23, 1, 10);
    if (bars > 10) {
      bars = 10;
    }

    // Serial.println(bars);
    if (level > 0)
      for (byte i = 0; i <= 10; i++) {
        if (i < bars) lcd.write((uint8_t)0);
        else lcd.print(F(" "));
      }
  }

  /**
      * Display home screen
      */
  void home(DrawInterface *dr) {

    lcd.setCursor(0, 0);
    lcd.print(F("Tank1"));
    if (ctrlWell.isTerminated())
      lcd.write((char)5);
    else
      lcd.print(F(" "));

    int level1 = read->getWellLevel();

    if (level1 == 0 || level1 > LevelSensorWellMin)
      lcd.print(F("[-?-]     "));
    else
      drawLevel(level1, LevelSensorWellMin);

    lcd.setCursor(0, 1);
    lcd.print(F("Tank2"));

    int level2 = read->getMainLevel();
    if (ctrlMain.isTerminated())
      lcd.write((char)5);
    else
      lcd.print(F(" "));

    if (level2 == 0 || level2 > LevelSensorMainMin)
      lcd.print(F("[-?-]     "));
    else
      drawLevel(level2, LevelSensorMainMin);
  }

  /**
     * Menu tank well
     * @param dr
     */
  void menuWell(DrawInterface *dr) {
    dr->edit(this->modeWell);
    lcd.setCursor(0, 0);
    lcd.print(F("Pumping         "));
    lcd.setCursor(0, 1);
    lcd.print(F("Mode: "));
    lcd.print(this->modeWell->getName());
    lcd.print(F("      "));
    lcd.setCursor(10, 1);
  }

  /**
     * Main tank menu
     * @param dr
     */
  void menuMain(DrawInterface *dr) {
    dr->edit(this->modeMain);
    lcd.setCursor(0, 0);
    lcd.print(F("Tank top        "));
    lcd.setCursor(0, 1);
    lcd.print(F("Start: "));
    lcd.print(this->modeMain->getName());
    lcd.print(F("      "));
    lcd.setCursor(11, 1);
  }

  /**
     * Pump menu Well tank
     * @param dr
     */
  void pumpWell(DrawInterface *dr) {

    dr->pump(&ctrlWell, &ctrlMain, true);

    lcd.setCursor(0, 0);
    lcd.print(F("Compressor:     "));

    lcd.setCursor(0, 1);
    lcd.print(F("         >> "));
    if (ctrlWell.isOn())
      lcd.print(F("ON  "));
    else
      lcd.print(F("OFF  "));
    lcd.setCursor(15, 1);
  }

  /**
     * Pumping menu Main tank
     * @param dr
     */
  void pumpMain(DrawInterface *dr) {
    dr->pump(&ctrlMain, &ctrlWell);

    lcd.setCursor(0, 0);
    lcd.print(F("Pump to up:     "));

    lcd.setCursor(0, 1);
    lcd.print(F("         >> "));
    if (ctrlMain.isOn())
      lcd.print(F("ON  "));
    else
      lcd.print(F("OFF  "));
    lcd.setCursor(15, 1);
  }

  /**
     * Info menu
     */
  void infoMenu() {
    lcd.setCursor(0, 0);
    if (time->isConn()) {
      // format 00-00 0000-00-00
      DateTime now = time->now();
      if (now.hour() < 10)
        lcd.print(F("0"));

      lcd.print(now.hour());

      if (time->tickClock())
        lcd.print(F(":"));  // simple ticking
      else lcd.print(F(" "));

      if (now.minute() < 10)
        lcd.print(F("0"));
      lcd.print(now.minute());

      lcd.print(F(" "));
      lcd.print(now.year());

      lcd.print(F("-"));
      if (now.month() < 10)
        lcd.print(F("0"));

      lcd.print(now.month());
      lcd.print(F("-"));

      if (now.day() < 10)
        lcd.print(F("0"));

      lcd.print(now.day());

    } else {
      lcd.setCursor(0, 0);
      lcd.print(F("No clock...     "));
    }


    lcd.setCursor(0, 1);
    if (time->isDaytime()) {
      lcd.write((char)2);
    } else {
      lcd.write((char)3);
    }
    lcd.print(F(" "));


    lcd.print(F("W"));  // well tank level cm
    lcd.print(formatUint8(read->getWellLevel()));
    lcd.print(F(" "));
    lcd.print(F("M"));  // main tank level cm
    lcd.print(formatUint8(read->getMainLevel()));
    lcd.print(F(" "));
    lcd.write((char)4);
    lcd.print(formatMsToTime(rule->getActionTimer()));
  }

  /**
     * Heat warning
     */
  void warnHeat() {
    lcd.setCursor(0, 0);
    lcd.print(F(" Overheating!   "));

    lcd.setCursor(0, 1);

    int temp = heat->getTemperature();

    lcd.print(formatNumTemp(temp));
    lcd.write((char)1);

    lcd.print(F(" FAN:"));
    lcd.print(formatUint8(heat->getFanSpeed()));
    lcd.print(F("   "));
  }

  /**
     * Heat information
     */
  void infoHeat() {
    lcd.setCursor(0, 0);

    if (time->isConn()) {
      lcd.print(formatNumTemp(time->getTemp()));
    } else lcd.print(F("---"));
    lcd.write((char)1);

    lcd.print(F(" "));
    lcd.print(F("SSR:"));
    lcd.print(formatNumTemp(heat->getTemperature()));
    lcd.write((char)1);
    lcd.print(F("   "));

    lcd.setCursor(0, 1);
    lcd.print(F(" Fan: "));
    lcd.print(formatUint8(heat->getFanSpeed()));
    lcd.print(F("       "));
  }

  void warnRule(DrawInterface *dr) {
    lcd.setCursor(0, 0);
    lcd.print(F(" Pump stopped..."));
    lcd.setCursor(0, 1);
    lcd.print(dr->getWarnMsg());
  }

  /**
     * Formats numbers to proper display format
     * @param value
     * @return
     */
  char *formatNumTemp(int value) {

    if (value < 0) {
      sprintf(formatBuffer, "-%02d", -value);
    } else
      sprintf(formatBuffer, " %02d", value);

    return formatBuffer;
  }

  /**
     * Formats uint8_t numbers to be displayed.
     * @param value
     * @return
     */
  char *formatUint8(uint8_t value) {
    sprintf(formatBuffer, "%03d", value);
    return formatBuffer;
  }

  /**
     * Formats given ms to time
     * @param milliseconds
     * @return
     */
  char *formatMsToTime(unsigned long milliseconds) {
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    unsigned long remainingHours = hours % 24;
    unsigned long remainingMinutes = minutes % 60;
    unsigned long remainingSeconds = seconds % 60;

    // Determine the appropriate time unit
    char timeUnit;
    int timeValue;
    if (days > 0) {
      timeValue = days;
      timeUnit = 'd';
    } else if (hours > 0) {
      timeValue = remainingHours;
      timeUnit = 'h';
    } else if (minutes > 0) {
      timeValue = remainingMinutes;
      timeUnit = 'm';
    } else {
      timeValue = remainingSeconds;
      timeUnit = 's';
    }

    sprintf(formatBuffer, "%02d%c", timeValue, timeUnit);
    return formatBuffer;
  }

  void handleEditState(DrawInterface *dr) {
    //
    // Еxecute only when is changed
    if (this->isEditLast != dr->isEditing()) {

      if (!dr->isEditing())
        lcd.noBlink();
      else
        lcd.blink();

      this->isEditLast = dr->isEditing();
    }
  }

public:


  //
  // Construct menu
  Menu(Rule *ru, Read *rd, Time *tm, Heat *ht, Data *mdW, Data *mdM)
    : rule(ru), read(rd), time(tm), heat(ht), modeWell(mdW), modeMain(mdM) {
  }

  void begin() {
    //
    // Start the display
    lcd.begin(16, 2);

    byte charBarLevel[8] = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B00000 };
    byte charCelsius[8] = { B00000, B01000, B00011, B00100, B00100, B00100, B00011, B00000 };
    byte charDayIcon[8] = { B00000, B10101, B01010, B10001, B01010, B10101, B00000, B00000 };
    byte charNightIcon[8] = { B00000, B01110, B10101, B11011, B10101, B01110, B00000, B00000 };
    byte charClockIcon[8] = { B00000, B01110, B10101, B10111, B10001, B01110, B00000, B00000 };
    byte charTerminate[8] = { B10100, B01000, B10100, B00000, B00000, B00000, B00000, B00000 };

    lcd.createChar(0, charBarLevel);
    lcd.createChar(1, charCelsius);
    lcd.createChar(2, charDayIcon);
    lcd.createChar(3, charNightIcon);
    lcd.createChar(4, charClockIcon);
    lcd.createChar(5, charTerminate);



    //
    // Print a Welcome message to the LCD.
    lcd.setCursor(0, 0);
    lcd.print(F("Automated"));
    lcd.setCursor(0, 1);
    lcd.print(F("  Water system "));
    lcd.blink();
    delay(1000);
  }


  /**
 * Draw the menu
 * @param dr
 */
  void draw(DrawInterface *dr) {

    //
    // Controls display blink state safe
    this->handleEditState(dr);
    //
    // Fresh tank levels
    if (dr->getCursor() == 0 && dr->isDisplayOn()) {
      read->startWorkRead();
    } else
      read->stopWorkRead();  // Stop fast read

    switch (dr->getCursor()) {

      case 0:
      default:
        this->home(dr);
        dr->resetCursor();
        break;

      case 1:
        return this->menuWell(dr);
      case 2:
        return this->menuMain(dr);

      case 5:
        return this->pumpWell(dr);
      case 6:
        return this->pumpMain(dr);
      case WarnMenu_Heat:
        return this->warnHeat();
      case WarnMenu_Rule:
        return this->warnRule(dr);


        //
        // Backward menu 2
      case 254:
        return this->infoHeat();

        //
        // Backward menu 1
      case 255:
        return this->infoMenu();
    }
  }
};

#endif