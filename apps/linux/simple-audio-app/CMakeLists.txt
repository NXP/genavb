cmake_minimum_required(VERSION 3.10)

project(simple-audio-app)

include_directories(${GENAVB_INCLUDE_DIR})

add_executable(${PROJECT_NAME}
  main.c
  ../common/ts_parser.c
  ../common/file_buffer.c
  ../common/common.c
  ../common/stats.c
  ../common/time.c
  ../common/msrp.c
  ../common/log.c
)

target_compile_options(${PROJECT_NAME} PUBLIC -O2 -Wall -Werror -g)

if(DEFINED GENAVB_LIB_DIR)
  add_library(genavb SHARED IMPORTED)
  set_target_properties(genavb PROPERTIES IMPORTED_LOCATION "${GENAVB_LIB_DIR}/libgenavb.so")
endif()

target_link_libraries(${PROJECT_NAME} genavb)

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
