cmake_minimum_required(VERSION 3.16)
project(fast_packet_protocol_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(NMEA2KFastPacketTarget main.cpp)

set_target_properties(
  NMEA2KFastPacketTarget
  PROPERTIES CXX_STANDARD 14
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  NMEA2KFastPacketTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                 Threads::Threads isobus::Utility)
