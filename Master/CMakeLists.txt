# CLion project for Master (Arduino Mega 2560).
# Open this Master/ folder as the CLion project root.
#
# Autocomplete: index target wires Arduino core + variant + libraries.
# Build/upload: custom targets call arduino-cli (see cmake/ArduinoClion.cmake).
#
# Env overrides (optional):
#   ARDUINO15            Arduino15 data dir
#   ARDUINO_LIBRARIES    sketchbook libraries dir
#   ARDUINO_SKETCHBOOK   parent of libraries/
#   ARDUINO_CLI          path to arduino-cli
#   ARDUINO_PORT         upload port (/dev/ttyACM0, COM3, ...)

cmake_minimum_required(VERSION 3.16)
project(Master LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)

get_filename_component(WS_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" ABSOLUTE)
include("${WS_CMAKE_DIR}/ArduinoClion.cmake")

file(GLOB_RECURSE MASTER_HEADERS
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h"
)

set(MASTER_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Master.ino"
  ${MASTER_HEADERS}
)

# CLion / clangd indexing (ATmega2560 + mega variant)
arduino_clion_add_index_target(Master_index __AVR_ATmega2560__ mega
  SOURCES ${MASTER_SOURCES}
)

# Real firmware: arduino:avr:mega / ATmega2560 (default CLion build target)
arduino_clion_add_cli_targets("arduino:avr:mega:cpu=atmega2560" DEFAULT)

# Host unit tests (optional; same as Master/tests/Makefile)
add_custom_target(host_tests
  COMMAND ${CMAKE_MAKE_PROGRAM} test
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  USES_TERMINAL
  COMMENT "Run Master host unit tests"
)
