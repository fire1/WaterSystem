# Shared helpers for CLion Arduino sketch projects (indexing + arduino-cli).
# Included from Master/CMakeLists.txt and Slave/CMakeLists.txt when each
# folder is opened as its own CLion project root.

cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# Locate Arduino15 package directory (OS + env aware)
# ---------------------------------------------------------------------------
function(arduino_clion_find_arduino15 out_var)
  if(DEFINED ENV{ARDUINO15} AND EXISTS "$ENV{ARDUINO15}")
    set(${out_var} "$ENV{ARDUINO15}" PARENT_SCOPE)
    return()
  endif()
  if(DEFINED ENV{ARDUINO_DIRECTORIES_DATA} AND EXISTS "$ENV{ARDUINO_DIRECTORIES_DATA}")
    set(${out_var} "$ENV{ARDUINO_DIRECTORIES_DATA}" PARENT_SCOPE)
    return()
  endif()

  set(_candidates)
  if(WIN32)
    if(DEFINED ENV{LOCALAPPDATA})
      list(APPEND _candidates "$ENV{LOCALAPPDATA}/Arduino15")
    endif()
    if(DEFINED ENV{USERPROFILE})
      list(APPEND _candidates "$ENV{USERPROFILE}/AppData/Local/Arduino15")
    endif()
  elseif(APPLE)
    list(APPEND _candidates "$ENV{HOME}/Library/Arduino15")
  endif()
  # Linux / WSL / snap fallbacks
  list(APPEND _candidates
    "$ENV{HOME}/.arduino15"
    "$ENV{HOME}/snap/arduino-cli/current/.arduino15"
  )

  foreach(_c ${_candidates})
    if(EXISTS "${_c}/packages/arduino/hardware/avr")
      set(${out_var} "${_c}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  set(${out_var} "" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Locate sketchbook / user libraries directory
# ---------------------------------------------------------------------------
function(arduino_clion_find_user_libraries out_var)
  if(DEFINED ENV{ARDUINO_LIBRARIES} AND EXISTS "$ENV{ARDUINO_LIBRARIES}")
    set(${out_var} "$ENV{ARDUINO_LIBRARIES}" PARENT_SCOPE)
    return()
  endif()
  if(DEFINED ENV{ARDUINO_SKETCHBOOK} AND EXISTS "$ENV{ARDUINO_SKETCHBOOK}/libraries")
    set(${out_var} "$ENV{ARDUINO_SKETCHBOOK}/libraries" PARENT_SCOPE)
    return()
  endif()

  set(_candidates)
  if(WIN32)
    if(DEFINED ENV{USERPROFILE})
      list(APPEND _candidates
        "$ENV{USERPROFILE}/Documents/Arduino/libraries"
        "$ENV{USERPROFILE}/Arduino/libraries"
      )
    endif()
    if(DEFINED ENV{HOMEPATH})
      list(APPEND _candidates "$ENV{HOMEPATH}/Documents/Arduino/libraries")
    endif()
  elseif(APPLE)
    list(APPEND _candidates "$ENV{HOME}/Documents/Arduino/libraries")
  endif()
  list(APPEND _candidates
    "$ENV{HOME}/Arduino/libraries"
    "$ENV{HOME}/Documents/Arduino/libraries"
  )

  foreach(_c ${_candidates})
    if(IS_DIRECTORY "${_c}")
      set(${out_var} "${_c}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  set(${out_var} "" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Pick newest installed arduino:avr core and matching avr-gcc tree
# ---------------------------------------------------------------------------
function(arduino_clion_resolve_avr arduino15_dir out_core out_avr_include out_avr_gcc_root)
  set(_avr_hw "${arduino15_dir}/packages/arduino/hardware/avr")
  if(NOT IS_DIRECTORY "${_avr_hw}")
    set(${out_core} "" PARENT_SCOPE)
    set(${out_avr_include} "" PARENT_SCOPE)
    set(${out_avr_gcc_root} "" PARENT_SCOPE)
    return()
  endif()

  file(GLOB _core_versions LIST_DIRECTORIES true "${_avr_hw}/*")
  list(SORT _core_versions)
  list(REVERSE _core_versions)
  set(_core "")
  foreach(_v ${_core_versions})
    if(EXISTS "${_v}/cores/arduino/Arduino.h")
      set(_core "${_v}")
      break()
    endif()
  endforeach()

  set(_gcc_root "")
  set(_avr_inc "")
  file(GLOB _gcc_versions LIST_DIRECTORIES true
    "${arduino15_dir}/packages/arduino/tools/avr-gcc/*")
  list(SORT _gcc_versions)
  list(REVERSE _gcc_versions)
  foreach(_g ${_gcc_versions})
    if(EXISTS "${_g}/avr/include/avr/io.h")
      set(_gcc_root "${_g}")
      set(_avr_inc "${_g}/avr/include")
      break()
    elseif(EXISTS "${_g}/include/avr/io.h")
      set(_gcc_root "${_g}")
      set(_avr_inc "${_g}/include")
      break()
    endif()
  endforeach()

  set(${out_core} "${_core}" PARENT_SCOPE)
  set(${out_avr_include} "${_avr_inc}" PARENT_SCOPE)
  set(${out_avr_gcc_root} "${_gcc_root}" PARENT_SCOPE)
endfunction()

function(arduino_clion_collect_lib_includes root out_list)
  set(_incs)
  if(NOT IS_DIRECTORY "${root}")
    set(${out_list} "${_incs}" PARENT_SCOPE)
    return()
  endif()
  file(GLOB _libs LIST_DIRECTORIES true "${root}/*")
  foreach(_lib ${_libs})
    if(IS_DIRECTORY "${_lib}")
      list(APPEND _incs "${_lib}")
      if(EXISTS "${_lib}/src")
        list(APPEND _incs "${_lib}/src")
      endif()
      if(EXISTS "${_lib}/utility")
        list(APPEND _incs "${_lib}/utility")
      endif()
    endif()
  endforeach()
  set(${out_list} "${_incs}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Indexing target for CLion / clangd autocomplete (host compiler, no link)
#   name        - CMake target name
#   mcu_define  - e.g. __AVR_ATmega2560__ or __AVR_ATmega8__
#   variant     - mega | standard
#   SOURCES     - sketch sources
# ---------------------------------------------------------------------------
# arduino_clion_add_index_target(<name> <mcu_define> <variant> SOURCES ... [NO_USB])
function(arduino_clion_add_index_target name mcu_define variant)
  set(options NO_USB)
  set(oneValueArgs)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(ACL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  arduino_clion_find_arduino15(_a15)
  arduino_clion_find_user_libraries(_user_libs)
  arduino_clion_resolve_avr("${_a15}" _core _avr_inc _gcc_root)

  if(NOT _core)
    message(WARNING
      "Arduino AVR core not found under Arduino15. "
      "Set ARDUINO15 or run: arduino-cli core install arduino:avr. "
      "CLion indexing includes will be incomplete.")
  else()
    message(STATUS "Arduino15: ${_a15}")
    message(STATUS "AVR core:  ${_core}")
    message(STATUS "avr-gcc:   ${_gcc_root}")
  endif()
  if(_user_libs)
    message(STATUS "User libs: ${_user_libs}")
  endif()

  # OBJECT library: compiles for compile_commands.json, never links.
  add_library(${name} OBJECT ${ACL_SOURCES})
  set_target_properties(${name} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
  )

  foreach(_src ${ACL_SOURCES})
    if(_src MATCHES "\\.ino$")
      set_source_files_properties("${_src}" PROPERTIES LANGUAGE CXX)
    endif()
  endforeach()

  set(_defs
    ${mcu_define}
    ARDUINO=10819
    F_CPU=16000000L
    __AVR__
    ARDUINO_ARCH_AVR
  )
  if(NOT ACL_NO_USB)
    list(APPEND _defs USBCON)
  endif()
  target_compile_definitions(${name} PRIVATE ${_defs})

  target_include_directories(${name} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
  )

  if(_core)
    target_include_directories(${name} PRIVATE
      "${_core}/cores/arduino"
      "${_core}/variants/${variant}"
    )
    arduino_clion_collect_lib_includes("${_core}/libraries" _core_libs)
    if(_core_libs)
      target_include_directories(${name} PRIVATE ${_core_libs})
    endif()
  endif()

  if(_avr_inc)
    target_include_directories(${name} PRIVATE "${_avr_inc}")
  endif()

  if(_user_libs)
    arduino_clion_collect_lib_includes("${_user_libs}" _ulibs)
    if(_ulibs)
      target_include_directories(${name} PRIVATE ${_ulibs})
    endif()
  endif()

  if(_a15 AND IS_DIRECTORY "${_a15}/libraries")
    arduino_clion_collect_lib_includes("${_a15}/libraries" _a15libs)
    if(_a15libs)
      target_include_directories(${name} PRIVATE ${_a15libs})
    endif()
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Real firmware build/upload via arduino-cli (CLion custom targets)
# Env:
#   ARDUINO_CLI       - path to arduino-cli (optional)
#   ARDUINO_PORT      - upload serial port
#   ARDUINO_LIBRARIES - sketchbook libraries dir for --library
# ---------------------------------------------------------------------------
# arduino_clion_add_cli_targets(<fqbn> [DEFAULT])
# DEFAULT → firmware is part of the default CLion/CMake build.
function(arduino_clion_add_cli_targets fqbn)
  cmake_parse_arguments(ACL "DEFAULT" "" "" ${ARGN})

  find_program(ARDUINO_CLI arduino-cli
    HINTS
      ENV ARDUINO_CLI
      "$ENV{HOME}/bin"
      /usr/local/bin
      /usr/bin
  )

  set(_port "$ENV{ARDUINO_PORT}")

  if(NOT ARDUINO_CLI)
    message(WARNING "arduino-cli not found; skipping compile/upload targets. "
                    "Install arduino-cli or set ARDUINO_CLI.")
    return()
  endif()

  arduino_clion_find_user_libraries(_user_libs)

  set(_lib_args)
  if(_user_libs)
    list(APPEND _lib_args --library "${_user_libs}")
  endif()

  message(STATUS "arduino-cli: ${ARDUINO_CLI}")
  message(STATUS "FQBN: ${fqbn}")
  if(_port)
    message(STATUS "Upload port (ARDUINO_PORT): ${_port}")
  else()
    message(STATUS "Upload port: unset (export ARDUINO_PORT=/dev/ttyUSB0 or COMx)")
  endif()

  add_custom_target(arduino-compile
    COMMAND "${ARDUINO_CLI}" compile -b "${fqbn}" ${_lib_args}
            "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "arduino-cli compile ${fqbn}"
  )

  if(ACL_DEFAULT)
    add_custom_target(firmware ALL
      DEPENDS arduino-compile
      COMMENT "Build firmware with arduino-cli"
    )
  else()
    add_custom_target(firmware
      DEPENDS arduino-compile
      COMMENT "Build firmware with arduino-cli"
    )
  endif()

  if(_port)
    add_custom_target(arduino-upload
      COMMAND "${ARDUINO_CLI}" compile -b "${fqbn}" ${_lib_args}
              "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMAND "${ARDUINO_CLI}" upload -b "${fqbn}" -p "${_port}"
              "${CMAKE_CURRENT_SOURCE_DIR}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      USES_TERMINAL
      COMMENT "arduino-cli upload ${fqbn} -> ${_port}"
    )
  else()
    add_custom_target(arduino-upload
      COMMAND ${CMAKE_COMMAND} -E echo
        "Set ARDUINO_PORT (e.g. /dev/ttyACM0, /dev/ttyUSB0, COM3) and reload CMake."
      COMMENT "arduino-upload requires ARDUINO_PORT"
    )
  endif()
endfunction()
