#ifndef Cmd_h
#define Cmd_h
//
// Class responsible for commands from serial monitor
//

#include "Glob.h"

const char CMD_SPR_EOL = '\n';

class Cmd {
private:

  String cmdName;
  String cmdData;
  bool isPrint = false;


public:
  Cmd() {}
  void hark() {

    //
    // Default message

    String output = F(" Command not found, use \"help=\" for instructions! ");
    isPrint = false;

    //
    // Execute command from serial
    if (Serial.available() > 0) {


      cmdName = Serial.readStringUntil('=');

      if (cmdName.length() > 3) {
        cmdData = Serial.readStringUntil(CMD_SPR_EOL);
        isPrint = true;
      }

      //
      // Help
      if (cmdName == F("help")) {

        Serial.println();
        Serial.println(F("Set Clock time and date:"));
        Serial.println(F("\t time=<00:00>  Sets clock time."));
        Serial.println(F("\t date=<0000-00-00>  Sets clock date."));
        Serial.println(F("\t show=<100>  Shows the clock"));
        Serial.println();
      }

      if (cmdName == F("show")) {
        switch (cmdData.toInt()) {
          case 100:
            DateTime now = RTClib::now();
            Serial.print(now.year(), DEC);
            Serial.print('/');
            Serial.print(now.month(), DEC);
            Serial.print('/');
            Serial.print(now.day(), DEC);
            Serial.print(' ');
            Serial.print(now.hour(), DEC);
            Serial.print(':');
            Serial.print(now.minute(), DEC);
            Serial.print(':');
            Serial.print(now.second(), DEC);
            Serial.println();
            break;
        }
      }

      //
      // Clock time
      if (cmdName == F("time")) {
        int hours = cmdData.substring(0, 2).toInt();
        int minutes = cmdData.substring(3, 5).toInt();

        Serial.print(F("Hours: "));
        Serial.println(hours);
        Serial.print(F("Minutes: "));
        Serial.println(minutes);

        output = F("Done!");
      }

      //
      // Clock date
      if (cmdName == F("date")) {
        int year = cmdData.substring(0, 4).toInt();
        int month = cmdData.substring(5, 7).toInt();
        int day = cmdData.substring(8).toInt();
        Serial.print(F("Year: "));
        Serial.println(year);
        Serial.print(F("Month: "));
        Serial.println(month);
        Serial.print(F("Day: "));
        Serial.println(day);

        output = F("Done!");
      }


      if (isPrint) {
        Serial.println("");
        Serial.println(output);
      }
    }
  }
};




#endif