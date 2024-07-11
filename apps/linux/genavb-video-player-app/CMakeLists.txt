cmake_minimum_required(VERSION 3.10)

project(genavb-video-player-app)

option(WAYLAND_BACKEND "Build application with support for wayland backend" ON)

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${GENAVB_INCLUDE_DIR})

add_executable(${PROJECT_NAME}
  main.c
  gstreamer.c
  gst_pipelines.c
  gstreamer_single.c
  ../common/common.c
  ../common/stats.c
  ../common/time.c
  ../common/ts_parser.c
  ../common/file_buffer.c
)

if(WAYLAND_BACKEND)
  target_compile_definitions(${PROJECT_NAME} PUBLIC WL_BUILD)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -O2 -Wall -Werror -g)

if(DEFINED GENAVB_LIB_DIR)
  add_library(genavb SHARED IMPORTED)
  set_target_properties(genavb PROPERTIES IMPORTED_LOCATION "${GENAVB_LIB_DIR}/libgenavb.so")
endif()

target_link_libraries(${PROJECT_NAME} genavb)
target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
install(PROGRAMS salsa-camera.sh DESTINATION usr/bin)
