cmake_minimum_required(VERSION 3.10)

project(GenAVB/TSN)

set(TOPDIR ${CMAKE_CURRENT_SOURCE_DIR})

set(VERSION_FILE common/version.h)

add_custom_target(stack)

if(NOT DEFINED TARGET)
  message(FATAL_ERROR "Target not defined")
endif()
message(STATUS "TARGET: ${TARGET}")

set(target_file "${CMAKE_CURRENT_SOURCE_DIR}/config_${TARGET}.cmake")
if(NOT EXISTS ${target_file})
  message(FATAL_ERROR "cannot find ${target_file}")
endif()

set(CMAKE_INSTALL_MESSAGE LAZY)
# If not explicetly defined, set install prefix to build directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/target"
        CACHE PATH "default install path" FORCE)
endif()

include(configs/configs.cmake)
include(${target_file})
include(extensions.cmake)

list(APPEND maindirs
  ${TARGET_OS}
  management
  gptp
  srp
  maap
  hsr
  avtp
  avdecc
  api
  public
  common
)

foreach(dir ${maindirs})
  include(${dir}/${dir}.cmake)
endforeach()

include(CMakeLists.${TARGET_OS})
include(doc/CMakeLists.txt)

genavb_generate_version()

genavb_generate_archives()

genavb_install()
