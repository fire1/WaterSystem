#include "HardwareSerial.h"


#ifndef Tone_h
#define Tone_h

#include "Arduino.h"
#include <AsyncDelay.h>

struct Note {
  int frequency;
  int duration;  // in milliseconds
};

class Tone {
private:
  const Note* currentMelody = nullptr;
  uint8_t currentNoteIndex = 0;
  unsigned long noteStartTime = 0;

  // Define Melodies
  const Note MelodyClick[2] = {
    { 659, 5 },  // Note frequency and duration (in milliseconds)
    { -1, 0 }     // End of melody marker
  };

  const Note MelodyMode[4] = {
    { 900, 50 },
    { 0, 50 },
    { 900, 50 },
    { -1, 0 }
  };

  const Note MelodyEnter[4] = {
    { 659, 100 },
    { 988, 50 },
    { -1, 0 }
  };

  const Note MelodySave[5] = {
    { 659, 100 },  // Note frequency and duration (in milliseconds)
    { 988, 50 },
    { 0, 100 },
    { 1200, 50 },
    { -1, 0 }  // End of melody marker
  };

public:
  Tone() = default;

  void begin() {
    pinMode(pinTone, OUTPUT);
  }

  void playMelody(const Note* melody) {
    currentMelody = melody;
    currentNoteIndex = 0;
    noteStartTime = millis();
    playCurrentNote();
  }

  void click() {
    playMelody(MelodyClick);
  }

  void enter() {
    playMelody(MelodyEnter);
  }

  void save() {
    playMelody(MelodySave);
  }

  void mode() {
    playMelody(MelodyMode);
  }

  void hark() {
    if (currentMelody != nullptr && millis() - noteStartTime >= (currentMelody + currentNoteIndex)->duration) {
      currentNoteIndex++;
      playCurrentNote();
    }
  }

private:
  void playCurrentNote() {
    if (currentMelody != nullptr && (currentMelody + currentNoteIndex)->frequency != -1) {
      tone(pinTone, (currentMelody + currentNoteIndex)->frequency);
      noteStartTime = millis();
    } else {
      noTone(pinTone);
      currentMelody = nullptr;
      currentNoteIndex = 0;
    }
  }
};


#endif
