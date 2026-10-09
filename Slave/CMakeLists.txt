# CLion project for Slave (ATmega8, Arduino NG / Uno-style wiring).
# Open this Slave/ folder as the CLion project root.
#
# Env overrides (optional): same as Master/CMakeLists.txt
#   ARDUINO15, ARDUINO_LIBRARIES, ARDUINO_SKETCHBOOK, ARDUINO_CLI, ARDUINO_PORT

cmake_minimum_required(VERSION 3.16)
project(Slave LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)

get_filename_component(WS_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" ABSOLUTE)
include("${WS_CMAKE_DIR}/ArduinoClion.cmake")

set(SLAVE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Slave.ino"
  "${CMAKE_CURRENT_SOURCE_DIR}/Lib.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/SlaveLogic.h"
)

# CLion / clangd indexing (ATmega8 + standard variant, Uno/NG pin map)
arduino_clion_add_index_target(Slave_index __AVR_ATmega8__ standard
  SOURCES ${SLAVE_SOURCES}
  NO_USB
)

# Real firmware: Arduino NG or older → ATmega8
# Note: stock SoftwareSerial does not build for ATmega8 (no PCMSK*). Indexing
# still targets ATmega8; use a TX-only bit-bang or ISP toolchain if needed.
arduino_clion_add_cli_targets("arduino:avr:atmegang:cpu=atmega8")

add_custom_target(host_tests
  COMMAND ${CMAKE_MAKE_PROGRAM} test
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  USES_TERMINAL
  COMMENT "Run Slave host unit tests"
)
