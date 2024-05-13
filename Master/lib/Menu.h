#include "WString.h"

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
  bool isEditLast = true;  // TRUE - Since welcome message begin with blink this will strop it.

  /**
        * Draw level from 0 to 10 bars
        * @param level The level of the tank, from 100 /empty/ and 20 /full/
        * @param min An accured min value /100/
        */
  void drawLevel(byte level, byte min) {

    uint8_t bars = map(level, min - 5, LevelSensorBothMax + 1, 0, 10);
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
    dr->noEdit();
    lcd.setCursor(0, 0);
    lcd.print(F("Tank1"));
    if (ctrlWell.isTerminated())
      //lcd.write((char)5);
      lcd.write((char)235);  // buildin char
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
      //lcd.write((char)5);
      lcd.write((char)235);  // buildin char
    else
      lcd.print(F(" "));

    if (level2 == 0 || level2 > LevelSensorMainMin)
      lcd.print(F("[-?-]     "));
    else
      drawLevel(level2, LevelSensorMainMin);
  }

  /**
     * Home screen version 2
     * @param dr
     */
  void homeV2(DrawInterface *dr) {
    dr->noEdit();
    lcd.setCursor(0, 0);
    uint8_t level1 = read->getWellLevel();
    lcd.write((char)6);

    // daytime mark
    if (time->isDaytime()) lcd.write((char)2);
    else lcd.write((char)3);

    lcd.print(formatUint8(level1));

    // Terminate mark
    if (ctrlWell.isTerminated())
      // lcd.write((char)5);
      lcd.write((char)235);
    else lcd.print(F(" "));

    if (level1 == 0 || level1 > LevelSensorWellMin)
      lcd.print(F("[-?-]     "));
    else
      drawLevel(level1, LevelSensorWellMin);
    //
    // Second row //////////////////////////////
    //
    lcd.setCursor(0, 1);
    uint8_t level2 = read->getMainLevel();
    lcd.write((char)7);
    lcd.print(F(" "));

    lcd.print(formatUint8(level2));

    // Terminate mark
    if (ctrlMain.isTerminated())
      // lcd.write((char)5);
      lcd.write((char)235);
    else lcd.print(F(" "));

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

    dr->pump(&ctrlWell, &ctrlMain);

    lcd.setCursor(0, 0);
    lcd.print(F("Compressor:     "));

    lcd.setCursor(0, 1);
    lcd.print(F("         >> "));
    if (ctrlWell.isOn())
      lcd.print(F("ON  "));
    else
      lcd.print(F("OFF "));
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
      lcd.print(F("OFF "));
    lcd.setCursor(15, 1);
  }

  /**
       * Info menu
       */
  void infoTime(DrawInterface *dr) {
    dr->noEdit();
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
    lcd.print(F(" "));
    lcd.write((char)4);
    if (time->isDaytime()) lcd.write((char)2);
    else lcd.write((char)3);
    lcd.print(F(" "));

    // When runs
    // Print to off time
    if (ctrlWell.isOn()) lcd.print(F("~"));
    else lcd.print(F(" "));                         //5
    lcd.print(formatMsToTime(rule->getNextOff()));  //8

    lcd.print(F(" "));  //9

    // When stopped
    // Print to on time
    if (!ctrlWell.isOn()) lcd.print(F("~"));
    else lcd.print(F(" "));  //10
    unsigned long next = rule->getNextOn();
    if (next > MaxDaysInMillis) {
      lcd.print(F("/"));
      lcd.write((char)243); // infinity 
      lcd.print(F("/"));
    } else lcd.print(formatMsToTime(rule->getNextOn()));  //13

    lcd.print(F("   "));
  }


  /**
       * Heat warning
       */
  void warnHeat(DrawInterface *dr) {
    dr->noEdit();
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
  void infoHeat(DrawInterface *dr) {
    dr->noEdit();
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
    dr->noEdit();
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

  /**
     * Shows blinking cursor
     * @param dr
     */
  void handleEditState(DrawInterface *dr) {
    //
    // Execute only when is changed
    if (this->isEditLast != dr->isEditing()) {
      if (dr->isEditing()) {
        lcd.blink();
      } else {
        lcd.noBlink();
      }

      this->isEditLast = dr->isEditing();
    }
  }

  void handleDebug() {
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
    byte charDayIcon[8] = { B00000, B00110, B01010, B01100, B00000, B00000, B00000, B00000 };
    byte charNightIcon[8] = { B00000, B00110, B01100, B00000, B00000, B00000, B00000, B00000 };
    byte charClockIcon[8] = { B00000, B01110, B10101, B10111, B10001, B01110, B00000, B00000 };
    byte charTerminate[8] = { B10100, B01000, B10100, B00000, B00000, B00000, B00000, B00000 };

    byte charOneLine[8] = { B00000, B01000, B11001, B01000, B01000, B01001, B11100, B11111 };
    byte charTwoLine[8] = { B00000, B01000, B10101, B00100, B01000, B10001, B11100, B11111 };

    lcd.createChar(0, charBarLevel);
    lcd.createChar(1, charCelsius);
    lcd.createChar(2, charDayIcon);
    lcd.createChar(3, charNightIcon);
    lcd.createChar(4, charClockIcon);
    lcd.createChar(5, charTerminate); // todo: free to replace since LCD has same charater
    lcd.createChar(6, charOneLine);
    lcd.createChar(7, charTwoLine);

    //   lcd.write((char)162); // - bracket 
    //   lcd.write((char)163); // - bracket 


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

    this->handleDebug();
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

      case MenuInfo_Home:
      default:
        //this->home(dr);
        this->homeV2(dr);
        dr->resetCursor();
        break;

      case MenuEdit_Well:
        return this->menuWell(dr);
      case MenuEdit_Main:
        return this->menuMain(dr);

      case MenuPump_Well:
        return this->pumpWell(dr);
      case MenuPump_Main:
        return this->pumpMain(dr);
      case MenuWarn_Heat:
        return this->warnHeat(dr);
      case MenuWarn_Rule:
        return this->warnRule(dr);


        //
        // Backward menu 2
      case MenuInfo_Heat:
        return this->infoHeat(dr);

        //
        // Backward menu 1
      case MenuInfo_Time:
        return this->infoTime(dr);
    }
  }
};

#endif