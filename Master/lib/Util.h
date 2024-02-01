#ifndef Util_h
#define Util_h

//
// Simple debug privider
#ifdef DEBUG
#define dbg(x) Serial.print(x)
#define dbgLn(x) Serial.println(x)
#else
#define dbg(x)
#define dbgLn(x)
#endif

//
// Macro to define PROGMEM strings
#define PSTR_DEF(name, value) static const char name[] PROGMEM = value

//
// Macro to define PROGMEM array
#define PROGMEM_ARRAY(name, ...) const char* PROGMEM name[] = {__VA_ARGS__}

// 
// Macro to define PROGMEM array with strings
#define DefineData(name, ...) \
  static const char* const name[] PROGMEM = {__VA_ARGS__}

//
// EepRome addresses
#define E_ADDR_MD // Address for Mode 
#define E_ADDR_TK1 0 // Address for Tank 1
#define E_ADDR_TK2 1 // Address for Tank 2

#endif