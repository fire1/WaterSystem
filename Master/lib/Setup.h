//
// Created by fire1 on 2024-03-06.
//

#ifndef WATERSYSTEM_SETUP_H
#define WATERSYSTEM_SETUP_H

//
// Definition setup
#define DEBUG  // Comment it to disable debugging
//#define DAYTIME_CHECK // Comment it to disable daytime check for running pumps
//#define WELL_MEASURE_DEFAULT // Uses trigger/echo to get distance (not recommended)
#define WELL_MEASURE_UART_47K // Uses Serial UART to communicate with the sensor
#define ENABLE_CMD// Enables Serial input listener for commands
#define ENABLE_CLOCK // Enables DS3231 clock usage

#endif //WATERSYSTEM_SETUP_H
