

#ifndef Global_h
#define Global_h

#include <stdint.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <AsyncDelay.h>
#include <Wire.h>
#include <CmdSerial.h>

#include "Setup.h"
//
// Used as debugging tool for 
//  Serial Input/Output.
CmdSerial cmd;

#define pinLed 13
#define pinTone 9
//
// Pump controll pins
#define pinWellPump A10  // Well pump pin putput
#define pinMainPump A11  // Main pump pin output
//
// Cooling control and monitoring
#define pinTmpRss A9  // Temperature input / NTC-MF52AT
#define pinFanRss 2   // Temperature fan for RSS
//
// Designed for AC loads has maximum junction temperature of 150°C
// Operating junction temperature range. -40 to +125. °C
#define stopMaxTemp 100
const int TempSampleReads = 10;


//
// Baud rate for Slave
#define BaudSlaveRx 4800

class Draw;

class Menu;

class Data;

class Pump;

class Tone;


class DrawInterface {
public:

    // Pure virtual functions - These functions must be implemented by derived classes
    virtual uint8_t getCursor() = 0;

    virtual void edit(Data *d) = 0;

    virtual void pump(Pump *p, Pump *s) = 0;

    virtual void resetCursor();

    virtual bool isEditing() = 0;

    virtual bool isDisplayOn() = 0;

    virtual void warn(uint8_t i, bool buzz = true) = 0;

    virtual void warn(uint8_t i, String msg) = 0;

    virtual String getWarnMsg() = 0;
};

const uint8_t PumpScheduleMaxIntervals = 8;
struct PumpSchedule {
    uint8_t workMin;
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
#define pinLedBeat 30
#define pinLedWell 31
#define pinLedMain 32

//
// Instant pump buttons
#define pinBtnWell 33
#define pinBtnMain 34


//
// Panel navigation pins
#define pinBtnBack 35
#define pinBtnOk 36
#define pinBtnNext 37

//
//  Ultrasonic Standart read mesurment pinns,
//    it is not recommended sice uses too much time to measure
#ifdef WELL_MEASURE_DEFAULT
#define pinWellEcho 15  // Echo pin
#define pinWellSend 14  // Trigger pin
#endif
//
// For this mode a 45Kohm resistor is solder for R19
#ifdef WELL_MEASURE_UART_47K   // For UART Serial I'm using Serial3 port RX3 pin 15 and TX3 pin 14
#define startUartCommand 0x01  // All available commands to transmit are 0x01, 0x00, 0x55
#define verifyCorrection 1     // Some of the sensors sends +/-  1/2 values, in my case is +1 in order to work verification.
#endif


#define LevelRefreshTimeIdle 1800000  // 30min
// #define LevelRefreshTimeIdle 900000  // 15min
#define LevelRefreshTimeWork 5000

#define pinMainPower 8  // Turn on (GND) power for slave
#define pinMainRx 10    // Recive data pin from slave
//
// Defines how meny time to read sensors
//  before defining tank state.
#define LevelSensorReads 5

//
// This value defines safe level
// point for max u-s sensor reads.
// Should be common for both sensors.
#define LevelSensorBothMax 20
#define LevelSensorMainMin 105

#define LevelSensorWellMin 110
#define LevelSensorStopWell 100

#define TimeoutPowerSlave 5000 // time to wait for powering up the main sensor

//
// Schedules for pumping well
const PumpSchedule ScheduleWellOnMainEasy = {12, 4, {80, 65, 50, 30}, {35, 360, 1440, 2880}};
const PumpSchedule ScheduleWellOnMainFast = {15, 3, {75, 50, 45}, {30, 60, 1440}};

#define SuspendDisplayTime 240000  // 4min
#define DisableSensorError 20
#define WarnScreenTimeout 5000

//
// Notification screens
const uint8_t WarnMenu_Heat = 7;
const uint8_t WarnMenu_Rule = 8;


//
// Debounce time for the joystick
#define BtnDebounceTime 10
#define BtnHoldTime 2000

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

extern Span spanSm(199);     // Loop span at Small
extern Span spanMd(2101);    // Loop span Middle  /screen refresh/
extern Span spanLg(5802);    // Loop span Large   /warning messages/
extern Span spanMx(250003);  // Loop span at 60k loops

#include "Time.h"
#include "Read.h"
#include "Rule.h"
#include "Heat.h"
#include "Menu.h"
#include "Draw.h"


#endif
