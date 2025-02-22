// Data.h
#ifndef Data_H
#define Data_H

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

class Data {
public:
    Data(uint8_t numOptions, const char *const *PROGMEM names, uint8_t eepromAddress)
            : numOptions(numOptions), eepromAddress(eepromAddress) {
        dataNames = names;
        readEepRom();
    }

    void next() {
        index = (index + 1) % numOptions;
        /*
        dbg(F("Index "));
        dbg(numOptions);
        dbg(' ');
        dbgLn(this->index);
        */
    }

    void back() {
        index = (index - 1 + numOptions) % numOptions;
        //dbgLn(this->index);
    }

    const char *getName() {
        return pgm_read_word(&(dataNames[index]));
    }

    uint8_t value() {
        return this->index;
    }

    void save() {
        //
        // Check for changes
        uint8_t value = static_cast<uint8_t>(index);
        if (stored != value)
            EEPROM.write(eepromAddress, value);
    }

    void setIndex(uint8_t index) {
        this->index = index;
        //  dbgLn(this->index);
    }

    //
    // Returns length all available options
    uint8_t length() {
        return this->numOptions;
    }

private:
    uint8_t stored;
    uint8_t numOptions;
    uint8_t eepromAddress;
    const char *const *PROGMEM dataNames;

    uint8_t index = 0;

    void readEepRom() {
        uint8_t storedValue = EEPROM.read(eepromAddress);
        stored = index = (storedValue < numOptions) ? storedValue : 0;
    }
};

#endif
