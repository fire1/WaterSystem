#ifndef Global_h
#define Global_h
/**
  * Gobal definition of object and paramerters
  *		This also is used as global include.
  */


#include <stdint.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <AsyncDelay.h>
#include <Wire.h>
#include <CmdSerial.h>

//
// Definition setup
#define DEBUG  // Comment it to disable debugging
//#define DAYTIME_CHECK // Comment it to disable daytime check for running pumps
//#define WELL_MEASURE_DEFAULT // Uses trigger/echo to get distance (not recommended)
#define WELL_MEASURE_UART_47K // Uses Serial UART to communicate with the sensor
#define ENABLE_CMD// Enables Serial input listener for commands
#define ENABLE_CLOCK // Enables DS3231 clock usage

//
// Used as debugging tool for
//  Serial Input/Output.
CmdSerial cmd;

const uint8_t pinLed = 13;
const uint8_t pinTone = 9;
//
// Pump controll pins
const uint8_t pinWellPump = A10;  // Well pump pin putput
const uint8_t pinMainPump = A11;  // Main pump pin output
//
// Cooling control and monitoring
const uint8_t pinTmpRss = A9;  // Temperature input / NTC-MF52AT
const uint8_t pinFanSsr = 2;   // Temperature fan for RSS
//
// Designed for AC loads has maximum junction temperature of 150°C
// Operating junction temperature range. -40 to +125. °C
const uint8_t stopMaxTemp = 90;
const uint8_t TempSampleReads = 10;


//
// Baud rate for Slave
#define BaudSlaveRx 4800

class Draw;

class Menu;

class Data;

class Pump;

class Tone;


//
// Draw Api interface
#include "DrIn.h"

//
// The max array len for  well pump schedule
const uint8_t PumpScheduleMaxIntervals = 4;

//
// The schedule structure
struct PumpSchedule {
  uint8_t runtime;
  uint8_t intervals;
  uint8_t levels[PumpScheduleMaxIntervals];
  uint16_t stops[PumpScheduleMaxIntervals];
};

//
// LCD setup
#define pinBacklight 29
const uint8_t
  pinRs = 22,
  pinEn = 23,
  pinD4 = 24,
  pinD5 = 25,
  pinD6 = 26,
  pinD7 = 27;
LiquidCrystal lcd(pinRs, pinEn, pinD4, pinD5, pinD6, pinD7);

//
// Panel led pins
const uint8_t pinLedBeat = 30;
const uint8_t pinLedWell = 31;
const uint8_t pinLedMain = 32;

//
// Instant pump buttons
const uint8_t pinBtnWell = 33;
const uint8_t pinBtnMain = 34;


//
// Panel navigation pins
const uint8_t pinBtnBack = 35;
const uint8_t pinBtnOk = 36;
const uint8_t pinBtnNext = 37;

//
//  Ultrasonic Standart read mesurment pinns,
//
//    it is not recommended sice uses too much time to measure tank level into the sketch.
#ifdef WELL_MEASURE_DEFAULT
const uint8_t pinWellEcho = 15;  // Echo pin
const uint8_t pinWellSend = 14;  // Trigger pin
#endif
// Default
// For this mode a 45Kohm resistor is solder for R19 pad on the PCB
#ifdef WELL_MEASURE_UART_47K   // For UART Serial I'm using Serial3 port RX3 pin 15 and TX3 pin 14
#define startUartCommand 0x01  // All available commands to transmit are 0x01, 0x00, 0x55
#define verifyCorrection 1     // Some of the sensors sends +/-  1/2 values, in my case is +1 in order to work verification.
#endif

// Sensor read tank level times
#define LevelRefreshTimeIdle 1800000  // 30min
#define LevelRefreshTimeWork 12000
#define TimeoutPowerSlave LevelRefreshTimeWork * 2  // time to wait for powering up the main sensor
//
// Sensors pins
const uint8_t pinMainPower = 8;  // Turn on (GND) power for slave
const uint8_t pinMainRx = 10;    // Recive data pin from slave
//
// Defines how meny time to read sensors
//  before defining tank state.
#define LevelSensorReads 5

//
// This value defines safe level
// point for max u-s sensor reads.
// Should be common for both sensors.
const uint8_t LevelSensorBothMax = 20;
const uint8_t LevelSensorMainMin = 105;
//
// Well levels
const uint8_t LevelSensorWellMin = 110;
const uint8_t LevelSensorStopWell = 90;

//
// Defining the best pumping runtime
const int8_t WellPumpDefaultRuntime = 12;
const int8_t WellPumpDefaultBreaktime = WellPumpDefaultRuntime * 1.7;
//
// Define maximum days in millis() check
const unsigned long MaxDaysInMillis = 40 * 24 * 60 * 60 * 1000;

//
// Schedules for well pumping periods by combining the levels of both tanks
//      FORMAT: {<on time>, <array length>, {<tank level>, ...}, {<off time>, ...} }
const PumpSchedule ScheduleWellEasy = { WellPumpDefaultRuntime * 0.84, 4, { 160, 130, 100, 60 }, { 35, 360, 1440, 2880 } };
const PumpSchedule ScheduleWellFast = { WellPumpDefaultRuntime, 3, { 150, 100, 90 }, { 30, 60, 1440 } };

//
// A clock time when to execute a dayjob for well pump	
const int8_t WellDayjobHour = 16;  // Clock hour

#define SuspendDisplayTime 240000  // After 4min will turn off the display. 
#define DisableSensorError 20  // How many errors will disable sensor read.
#define WarnScreenTimeout 5000 // Time to display warning message.

//
// Cursor map of the menu UI
const uint8_t MenuInfo_Home = 0;
const uint8_t MenuEdit_Well = 1;
const uint8_t MenuEdit_Main = 2;
const uint8_t MenuPump_Well = 5;
const uint8_t MenuPump_Main = 6;
const uint8_t MenuInfo_Time = 255;
const uint8_t MenuInfo_Heat = 254;
//
// Warn (notification) screens
const uint8_t MenuWarn_Heat = 7;
const uint8_t MenuWarn_Rule = 8;


//
// Debounce time for the joystick
#define BtnDebounceTime 10
#define BtnHoldTime 2000 // deprecated

//
// Constructing
#include "Pump.h"
#include "Util.h"
#include "Data.h"
#include "Span.h"
#include "Buzz.h"

//
// Pump controlling
extern Pump ctrlWell(pinWellPump, pinBtnWell, pinLedWell);
extern Pump ctrlMain(pinMainPump, pinBtnMain, pinLedMain);

extern Span spanSm(149);     // Loop span at Small
extern Span spanMd(250);     // Loop span Middle  /screen refresh/
extern Span spanLg(7093);    // Loop span Large   /warning messages/
extern Span spanMx(250005);  // Loop span at 60k loops

#include "Time.h"
#include "Read.h"
#include "Rule.h"
#include "Heat.h"
#include "Menu.h"
#include "Draw.h"


#endif
