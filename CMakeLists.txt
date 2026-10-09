# WaterSystem meta-project (optional). Prefer opening Master/ or Slave/
# directly in CLion for per-board indexing and arduino-cli targets.
cmake_minimum_required(VERSION 3.16)
project(WaterSystem LANGUAGES NONE)

add_subdirectory(Master)
add_subdirectory(Slave)
