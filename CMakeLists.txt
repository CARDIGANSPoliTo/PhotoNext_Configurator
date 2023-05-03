cmake_minimum_required(VERSION 2.8.9)
project (smartscanconfig)

set(smartscanconfig_VERSION_MAJOR 0)
set(smartscanconfig_VERSION_MINOR 1)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-Wall -std=c++11")

find_package (Threads REQUIRED)
find_library(LIBUTILS     utils)
find_library(LIBSMARTSCAN smartscan)

if ( NOT LIBUTILS )
  message(FATAL_ERROR "utils library not found")
endif()

if(NOT LIBSMARTSCAN)
  message(FATAL_ERROR "smartscan library not found")
endif()


include_directories(
  include
)

add_executable(smartscanconfig src/smartscanconfig.c)
target_link_libraries(smartscanconfig -lutils)
target_link_libraries(smartscanconfig -lsmartscan)
target_link_libraries(smartscanconfig ${CMAKE_THREAD_LIBS_INIT})

# install(TARGETS smartscanconfig DESTINATION bin)
