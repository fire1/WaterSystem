// Data.h
#ifndef Data_H
#define Data_H

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// Helper to get mode title without needing ModeInterface here
extern const __FlashStringHelper* getModeTitle(uint8_t idx);

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

    // Returns a RAM pointer to a space-padded name with given width
    const char* getName(uint8_t width) {
        static char buffer[17]; // LCD lines are 16 chars max
        if (width > 16) width = 16;

        // Prefill with spaces
        memset(buffer, ' ', width);
        buffer[width] = '\0';

        PGM_P src = NULL;
        if (dataNames) {
            // Read pointer to PROGMEM string
            return pgm_read_word(&(dataNames[index]));
        } else {
            if (index == 0) {
                src = PSTR("None");
            } else {
                const __FlashStringHelper* t = getModeTitle(index);
                if (t) src = (PGM_P)t;
            }
        }

        // Copy up to width bytes from PROGMEM, stop on NUL, preserve trailing spaces
        if (src) {
            for (uint8_t i = 0; i < width; ++i) {
                uint8_t c = pgm_read_byte(src + i);
                if (!c) break;
                buffer[i] = (char)c;
            }
        }

        buffer[width] = '\0';
        return buffer;
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
