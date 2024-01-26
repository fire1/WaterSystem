// Data.h
#ifndef Data_H
#define Data_H

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

class Data {
public:
  Data(uint8_t numOptions, const char* const* PROGMEM names, uint8_t eepromAddress)
    : numOptions(numOptions), eepromAddress(eepromAddress) {
    dataNames = names;
    readEepRom();
  }

  void next() {
    this->index = (index + 1) % numOptions;
  }

  void back() {
    this->index = (index - 1 + numOptions) % numOptions;
  }

  const char* getName() {
    return pgm_read_word(&(dataNames[index]));
  }

  uint8_t value() {
    return this->index;
  }

  void save() {
    EEPROM.write(eepromAddress, static_cast<uint8_t>(index));
  }

private:
  uint8_t numOptions;
  uint8_t eepromAddress;
  const char* const* PROGMEM dataNames;

  uint8_t index;

  void readEepRom() {
    uint8_t storedValue = EEPROM.read(eepromAddress);
    index = (storedValue < numOptions) ? storedValue : 0;
  }
};

#endif