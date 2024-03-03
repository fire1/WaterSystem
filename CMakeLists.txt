cmake_minimum_required(VERSION 2.8.4)
project(WaterSystem)


set(ARDUINO_CPU=atmega2560)
SET(MCU atmega2560)
SET(F_CPU 16000000)
set(CMAKE_C_FLAGS -mmcu=${MCU})
set(CMAKE_CXX_FLAGS  -mmcu=${MCU})
set(ARDUINO_SDK_PATH C:/Users/fire1/AppData/Local/Arduino15/packages/arduino/)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)

set(PROJECT_NAME WaterSystem)
project(${PROJECT_NAME})

set(${CMAKE_PROJECT_NAME}_BOARD mega)
set(${CMAKE_PROJECT_NAME}_PORT /dev/ttys3)

set(${CMAKE_PROJECT_NAME}_SKETCH Master/Master.ino)
#generate_arduino_firmware(${CMAKE_PROJECT_NAME})

set(BOARD_TAG mega)
set(BOARD_SUB atmega2560)

set(SOURCES Arduino.h)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${ARDUINO_SDK}/libraries)
#
# Enable syntax check for Serial class from UsbApi.h
# To set definitions use "-D"[DefName]
add_definitions( -DUSBCON=1 )
add_definitions ( -DSerial2=Serial)

link_directories("libraries")


link_directories($ENV{HOMEPATH}/Documents/Arduino/libraries)

