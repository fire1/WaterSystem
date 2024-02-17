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
Data tk1(5, tankNames, 0);
Data tk2(5, tankNames, 1);

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
Data md(4, modeNames, 2);


Tone tn;

//
// Initialize managment driver
Rule rl(&tn, &md, &tk1, &tk2);

//
// Menu UI instance
Menu mn(&rl, &tk1, &tk2, &md);

//
// Draw driver
Draw ui(&tn);

//
// Comands
Cmd cd;