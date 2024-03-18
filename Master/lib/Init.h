#ifndef Init_h
#define Init_h


//
// Define mode names
DefineData(
        modeNames,
"None",
"Easy",
"Fast",
"Now!");

//
// Convert names to object data
Data modeWellTank(4, modeNames, 2);

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

Data modeMainTank(5, tankNames, 1);

Time time;

Buzz buzz;

Read read;

Heat heat(&buzz);
//
// Initialize managment driver
Rule rule(&read, &time, &buzz, &modeWellTank, &modeMainTank);

//
// Menu UI instance
Menu menu(&rule, &read, &time, &heat, &modeWellTank, &modeMainTank);

//
// Draw driver
Draw draw(&read, &buzz);

//
// Comands
Cmd cmd(&time);

#endif