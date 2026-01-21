
//
// Initialize base objects

#ifndef Init_h
#define Init_h


//
// Mode selection is driven by ModeInterface instances in modes[]
// Data will store the chosen mode index in EEPROM; pass NULL for names so
// Data::getName() uses titles from the modes[] array.
Data modeWellTank(MODE_COUNT, nullptr, 2);

//
// Define tanks mode names
DefineData(
        tankNames,
"None",
"Full",
"Half",
"Void");

//
// Convert tank names  to data objects

Data modeMainTank(4, tankNames, 1);

Time time;

Buzz buzz;

Read read;

Heat heat(&buzz);

WinterMode winterMode;
Mode* modes[] = { nullptr, &winterMode };

// Return title for mode index (flash string). Keeps Data.h free of ModeInterface.
inline const __FlashStringHelper* getModeTitle(uint8_t idx) {
    if (idx < MODE_COUNT && modes[idx]) return modes[idx]->getTitleFlash();
    return (const __FlashStringHelper*)PSTR("");
}

//
// Initialize managment driver
Rule rule(&read, &time, &buzz, &modeWellTank, &modeMainTank);

//
// Menu UI instance
Menu menu(&rule, &read, &time, &heat, &modeWellTank, &modeMainTank);

//
// Draw driver
Draw draw(&read, &buzz);

#endif