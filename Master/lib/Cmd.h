#include "HardwareSerial.h"
#include <stdint.h>

#ifndef Cmd_h
#define Cmd_h
//
// Class responsible for commands from serial monitor
//

#include "Glob.h"

const char CMD_SPR_EOL = '\n';

class Cmd {
private:
    Time *time;
    String cmdName;
    String cmdData;
    bool isPrint = false;


public:
    Cmd(Time *t)
            : time(t) {}

    void hark(Read *read, Rule *rule) {

        //
        // Default message

        String output = F(" Command not found, use \"help=\" for instructions! ");
        isPrint = false;

        //
        // Execute command from serial
        if (Serial.available() > 0) {


            cmdName = Serial.readStringUntil(' ');

            if (cmdName.length() > 3) {
                cmdData = Serial.readStringUntil(CMD_SPR_EOL);
                cmdData.trim();
                isPrint = true;
            }

            //
            // Help
            if (cmdName == F("help")) {

                Serial.println();
                Serial.println(F("Show clock command:"));
                Serial.println(F("\t show clock"));

                Serial.println(F("Dump measurements:"));
                Serial.println(F("\t show main"));
                Serial.println(F("\t show well"));
                Serial.println();
            }


            if (cmdName == F("show")) {

                if (cmdData.equals(F("clock"))) {
                    DateTime now = time->now();
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
                    output = F("Clock dumped!");
                }

                if (cmdData.equals(F("main"))) {
                    output = F("Dumping main");

                    Serial.print(F(" Main: "));
                    Serial.print(read->getMainLevel());
                    Serial.println();

                }


                if (cmdData.equals(F("well"))) {
                    output = F("Dumping well");
                    Serial.print(F(" Well: "));
                    Serial.print(read->getWellLevel());
                    Serial.println();

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

            if (cmdName == F("well")) {
                int wellValue = (uint8_t) cmdData.substring(0, 2).toInt();
                read->setWell(wellValue);
                Serial.println(wellValue);
                output = F("Read /well/ overwritten!");
            }

            if (cmdName == F("main")) {
                int mainValue = (uint8_t) cmdData.substring(0, 2).toInt();
                read->setMain(mainValue);
                Serial.println(mainValue);
                output = F("Read /main/ overwritten!");
            }

            if (cmdName == F("cool")) {
                uint8_t pwm = (uint8_t) cmdData.toInt();
                rule->setFan(pwm);
                output = F(" Sets cooling fan to: ");
                output += rule->getFanSpeed();
            }

            if (cmdName == F("heat")) {
                uint8_t temp = (int) cmdData.toInt();
                rule->setHeat(temp);

                Serial.print(F("Seting heat at: "));
                Serial.println(temp);

                output = F(" Fan speed at: ");
                output += rule->getFanSpeed();
            }


            if (isPrint) {
                Serial.println("");
                Serial.println(output);
            }
        }
    }
};


#endif