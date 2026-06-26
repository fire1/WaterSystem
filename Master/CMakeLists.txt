cmake_minimum_required(VERSION 3.10)
project(WaterSystem CXX)

# 1. SET PATHS
set(ARDUINO_CORE_PATH "$ENV{HOME}/.arduino15/packages/arduino/hardware/avr/1.8.7")
set(AVR_TOOLCHAIN_PATH "$ENV{HOME}/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/avr/include")

# 2. DEFINE THE CHIP AND ARDUINO VERSION
add_definitions(-D__AVR_ATmega2560__ -DARDUINO=10800 -DF_CPU=16000000L -D__AVR__)

# 3. INCLUDE DIRECTORIES
include_directories(".")
include_directories("lib")

# Core Arduino Headers
include_directories(
        "${ARDUINO_CORE_PATH}/cores/arduino"
        "${ARDUINO_CORE_PATH}/variants/mega"
        "${AVR_TOOLCHAIN_PATH}"
)

# Core Arduino Libraries (SoftwareSerial, Wire, etc.)
file(GLOB CORE_LIBS "${ARDUINO_CORE_PATH}/libraries/*")
foreach(LIB_DIR ${CORE_LIBS})
    if(IS_DIRECTORY ${LIB_DIR})
        include_directories(${LIB_DIR})
        # Some libraries have a 'src' folder
        if(EXISTS "${LIB_DIR}/src")
            include_directories("${LIB_DIR}/src")
        endif()
    endif()
endforeach()

# User Arduino Libraries (AsyncDelay, RTClib, etc.)
file(GLOB USER_LIBS "$ENV{HOME}/Arduino/libraries/*")
foreach(LIB_DIR ${USER_LIBS})
    if(IS_DIRECTORY ${LIB_DIR})
        include_directories(${LIB_DIR})
        if(EXISTS "${LIB_DIR}/src")
            include_directories("${LIB_DIR}/src")
        endif()
    endif()
endforeach()

# Arduino15 Libraries (LiquidCrystal, etc.)
file(GLOB ARD15_LIBS "$ENV{HOME}/.arduino15/libraries/*")
foreach(LIB_DIR ${ARD15_LIBS})
    if(IS_DIRECTORY ${LIB_DIR})
        include_directories(${LIB_DIR})
        if(EXISTS "${LIB_DIR}/src")
            include_directories("${LIB_DIR}/src")
        endif()
    endif()
endforeach()

# 4. SOURCES
file(GLOB_RECURSE SOURCES 
    "*.ino" 
    "lib/*.h" 
    "lib/*.cpp"
)

# Tell CLion to treat .ino files as C++
set_source_files_properties(Master.ino PROPERTIES LANGUAGE CXX)

add_executable(WaterSystem ${SOURCES})
