#ifndef Init_h
#define Init_h

//
// Define tanks mode names
DefineData(
  tankNames,
  "None",
  "Full",
  "High",
  "Half",
  "Lows");

//
// Convert tank names  to data objects
Data tank1(5, tankNames, 0);
Data tank2(5, tankNames, 1);

//
// Define mode names
DefineData(
  modeNames,
  "None",
  "Easy",
  "Fast",
  "Now!");

Time time;

//
// Convert names to object data
Data mode(4, modeNames, 2);


Buzz buzz;

//
// Initialize managment driver
Rule rule(&buzz, &time, &mode, &tank1, &tank2);

//
// Menu UI instance
Menu menu(&rule, &time, &tank1, &tank2, &mode);

//
// Draw driver
Draw draw(&buzz);

//
// Comands
Cmd cmd(&time);

#endif