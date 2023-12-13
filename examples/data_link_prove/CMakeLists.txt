cmake_minimum_required(VERSION 3.16)
project(data_link_prove)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(DatLinkProveTarget main.cpp)

set_target_properties(
  DatLinkProveTarget
  PROPERTIES CXX_STANDARD 14
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  DatLinkProveTarget
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
