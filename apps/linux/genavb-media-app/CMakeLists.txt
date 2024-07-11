cmake_minimum_required(VERSION 3.10)

project(genavb-media-app)

option(WAYLAND_BACKEND "Build application with support for wayland backend" ON)

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${GENAVB_INCLUDE_DIR})

add_executable(${PROJECT_NAME}
  main.c
  thread_config.c
  alsa_config.c
  avb_stream_config.c
  gstreamer_stream.c
  multi_frame_sync.c
  salsa_camera.c
  h264_camera.c
  ../common/clock_domain.c
  ../common/thread.c
  ../common/alsa2.c
  ../common/clock.c
  ../common/log.c
  ../common/stats.c
  ../common/msrp.c
  ../common/avb_stream.c
  ../common/crf_stream.c
  ../common/alsa_stream.c
  ../common/stream_stats.c
  ../common/time.c
  ../common/gstreamer.c
  ../common/gstreamer_multisink.c
  ../common/gstreamer_single.c
  ../common/common.c
  ../common/ts_parser.c
  ../common/file_buffer.c
  ../common/avb_stream.c
  ../common/gst_pipeline_definitions.c
  ../common/aecp.c
  ../common/avdecc.c
  ../common/gstreamer_custom_rt_pool.c
  ../common/timer.c
  ../common/audio_mappings.c
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
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} dl)
target_link_libraries(${PROJECT_NAME} asound)
target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
