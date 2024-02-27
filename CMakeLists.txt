cmake_minimum_required(VERSION 3.27)
project(WaterSystem)

set(CMAKE_CXX_STANDARD 11)

set(ARDUINO_CPU=atmega2560)
set(MCU atmega2560)
set(F_CPU 16000000)
set(ARDUINO_SDK_PATH C:/Users/fire1/AppData/Local/Arduino15/packages/arduino/)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

set(PROJECT_NAME WaterSystem)
project(${PROJECT_NAME})

set(${CMAKE_PROJECT_NAME}_BOARD mega)
set(${CMAKE_PROJECT_NAME}_PORT /dev/ttys3)

set(${CMAKE_PROJECT_NAME}_SKETCH ./Master/Master.ino)
#generate_arduino_firmware(${CMAKE_PROJECT_NAME})

set(BOARD_TAG mega)
set(BOARD_SUB atmega2560)

set(SOURCES Arduino.h)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${ARDUINO_SDK}/libraries)
#
# Enable syntax check for Serial class from UsbApi.h
# To set definitions use "-D"[DefName]
add_definitions(-DUSBCON=1)
add_definitions(-DSerial2=Serial)

#target_include_directories("C:/Users/fire1/AppData/Local/Arduino15/packages/arduino/hardware/avr/1.8.6/libraries")
#target_include_directories("C:/Users/fire1/Documents/Arduino/libraries")

#[[
add_executable(
        WaterSystem

        Master/lib/Read.h
        Master/lib/Glob.h
        Master/lib/Read.h
        Master/Master.ino
)
]]
