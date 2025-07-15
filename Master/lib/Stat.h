#ifndef STAT_H
#define STAT_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>


/**
 * @brief Statistic recordin class for well pump
 *  Example how to use: stat.record(now(), pumpRunSeconds, pumpedCM, isHuman);
 * 
 */
class Stat {
private:
    const char* filePath = "/stats.csv";

public:
    void begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println(F("Failed to mount SPIFFS"));
        }
    }

    void record(uint32_t timestamp, uint16_t workTimeSec, uint16_t pumpedCM, bool isHuman) {
        File file = SPIFFS.open(filePath, FILE_APPEND);
        if (!file) return;
        file.printf("%lu,%u,%u,%d\n", timestamp, workTimeSec, pumpedCM, isHuman ? 1 : 0);
        file.close();
    }

    void printAllStats() {
        File file = SPIFFS.open(filePath, FILE_READ);
        if (!file) return;
        Serial.println("== Statistics ==");
        while (file.available()) {
            String line = file.readStringUntil('\n');
            Serial.println(line);
        }
        file.close();
    }

    uint32_t getMonthlyPumped(int month, int year) {
        File file = SPIFFS.open(filePath, FILE_READ);
        if (!file) return 0;

        uint32_t total = 0;
        while (file.available()) {
            String line = file.readStringUntil('\n');
            uint32_t ts;
            uint16_t work, cm;

            sscanf(line.c_str(), "%lu,%hu,%hu", &ts, &work, &cm);
            struct tm *timeinfo = localtime((time_t*)&ts);
            if (timeinfo->tm_mon + 1 == month && timeinfo->tm_year + 1900 == year)
                total += cm;
        }
        file.close();
        return total;
    }
};

#endif
