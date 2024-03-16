
#ifndef Menu_h
#define Menu_h

#include "Glob.h"

class Menu {
private:
    Rule *rule;
    Read *read;
    Time *time;
    Data *modeWell;
    Data *modeMain;
    bool isLevelReset = false;

    /**
      * Draw level from 0 to 10 bars
      * @param level The level of the tank, from 100 /empty/ and 20 /full/
      * @param min An accured min value /100/
      */
    void drawLevel(byte level, byte min) {

        uint8_t bars = map(level, min, LevelSensorBothMax, 1, 10);
        // uint8_t bars = map(level, 100, 23, 1, 10);
        if (bars > 10) {
            bars = 10;
        }

        // Serial.println(bars);
        if (level > 0)
            for (byte i = 0; i <= 10; i++) {
                if (i < bars) lcd.write((uint8_t) 0);
                else lcd.print(F(" "));
            }
    }

    /**
      * Display home screen
      */
    void home(DrawInterface *dr) {

        lcd.setCursor(0, 0);
        lcd.print(F("Tank1 "));
        int level1 = read->getWellLevel();

        if (level1 == 0 || level1 > LevelSensorWellMin)
            lcd.print(F("[-?-]     "));
        else
            drawLevel(level1, LevelSensorWellMin);

        lcd.setCursor(0, 1);
        lcd.print(F("Tank2 "));
        int level2 = read->getMainLevel();
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
        lcd.print(F("Pumpping        "));
        lcd.setCursor(0, 1);
        lcd.print(F("Mode: "));
        lcd.print(this->modeWell->getName());
        lcd.print(F("      "));
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
            lcd.print(F("OFF  "));

        lcd.blink();
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

        lcd.blink();
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
            lcd.print(F(" No clock...    "));
        }

        if (time->isDaytime()) {
            lcd.write((char) 2);
        } else {
            lcd.write((char) 3);
        }
        lcd.print(F(" "));

        lcd.setCursor(0, 1);
        lcd.print(F("W"));
        lcd.print(read->getWellLevel());

        lcd.setCursor(11, 1);
        lcd.print(F("M"));
        lcd.print(read->getMainLevel());
    }

/**
 * Heat warning
 */
    void warnHeat() {
        lcd.setCursor(0, 0);
        lcd.print(F(" Overheating!   "));

        int temp = rule->getHeat();
        lcd.setCursor(0, 1);
        lcd.print(F(" "));
        if (temp > -1 && temp < 10) lcd.print(F(" "));
        lcd.print(temp);
        lcd.write((char) 1);


        lcd.print(F(" FAN:"));
        lcd.print(rule->getFanSpeed());
    }

/**
 * Heat information
 */
    void infoHeat() {
        lcd.setCursor(0, 0);
        if (time->isConn()) {
            lcd.print(time->getTemp());
            lcd.write((char) 1);
        }

        lcd.setCursor(5, 0);
        lcd.print(F("SSR: "));
        lcd.print(formatNumTemp(rule->getHeat()));
        lcd.write((char) 1);

        lcd.setCursor(0, 1);
        lcd.print(F("Fan: "));
        lcd.print(rule->getFanSpeed());
    }

    /**
     * Formats numbers to proper display format
     * @param value
     * @return
     */
    char *formatNumTemp(int value) {
        char buffer[4];
        if (value < 0) {
            sprintf(buffer, "-%02d", -value);
        } else
            sprintf(buffer, "%03d", value);

        return buffer;
    }

public:
    //
    // Construct menu
    Menu(Rule *ru, Read *rd, Time *tm, Data *mdW, Data *mdM)
            : rule(ru), read(rd), time(tm), modeWell(mdW), modeMain(mdM) {
    }

    void begin() {
        //
        // Start the display
        lcd.begin(16, 2);

        byte charBarLevel[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B00000};
        byte charCelsius[8] = {B00000, B01000, B00011, B00100, B00100, B00100, B00011, B00000};
        byte charDayIcon[8] = {B00000, B10101, B01010, B10001, B01010, B10101, B00000, B00000};
        byte charNightIcon[8] = {B00000, B00100, B01110, B11011, B01110, B00100, B00000, B00000};

        lcd.createChar(0, charBarLevel);
        lcd.createChar(1, charCelsius);
        lcd.createChar(2, charDayIcon);
        lcd.createChar(3, charNightIcon);



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

        if (!dr->isEditing()) lcd.noBlink();
        else lcd.blink();
        //
        // Fresh tank levels
        if (dr->getCursor() == 0 && dr->isDisplayOn()) {
            read->startWorkRead();
        } else read->stopWorkRead();  // Stop fast read

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
            case 7:
                return this->warnHeat();

            case 254:
                return this->infoHeat();

                //
                // This menu is active only when clock is connected!
            case 255:
                return this->infoMenu();
        }
    }
};

#endif