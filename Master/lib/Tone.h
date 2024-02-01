
#ifndef Tone_h
#define Tone_h

// #include <toneAC.h>

struct Melody {
  uint8_t length;
  int notes[8];
  int durations[8];
};

#define MELODY_BOOT 0
#define MELODY_CLICK 1
#define MELODY_ENTER 2
#define MELODY_SAVE 3

// Define multiple melodies in PROGMEM
const Melody Melodies[] PROGMEM = {
  // Melody 0: Boot melody
  { 8, { 659, 587, 740, 784, 880, 880, 988, 880 }, { 4, 3, 4, 3, 4, 2, 4, 4 } },

  // Melody 1: Click sound
  { 1, { 2200 }, { 2 } },

  // Melody 2: Enter sound
  { 2, { 2200, 2300 }, { 2, 1 } },

  // Melody 2: Save sound
  { 4, { 1800, 1200, 2000, 2000 }, { 1, 1, 1, 1 } },
  // Add more melodies as needed
};

//
// Simple function to play the defined melodies
void playMelody(const Melody& melody) {
  for (int i = 0; i < melody.length; ++i) {
    int note = pgm_read_word_near(melody.notes + i);
    int duration = pgm_read_word_near(melody.durations + i);

    tone(pinTone, note);
    delay(100 / duration);
    noTone(pinTone);
    delay(25);
  }
}

void bootMelody() {
  int melody[] = { 262, 294, 330, 349, 392, 440, 494, 523 };
  int noteDurations[] = { 4, 4, 4, 4, 4, 4, 4, 4 };

  for (int i = 0; i < 8; ++i) {
    tone(pinTone, melody[i]);
    delay(500 / noteDurations[i]);
    noTone(pinTone);
    delay(50);  // Add a small delay between notes
  }
}

#endif