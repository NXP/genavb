cmake_minimum_required(VERSION 3.10)

project(alsa-audio-app)

include_directories(${GENAVB_INCLUDE_DIR})

add_executable(${PROJECT_NAME}
  main.c
  ../common/alsa.c
  ../common/stats.c
  ../common/time.c
  ../common/msrp.c
  ../common/log.c
  ../common/common.c
  ../common/avdecc.c
)

target_compile_definitions(${PROJECT_NAME} PUBLIC WL_BUILD)

target_compile_options(${PROJECT_NAME} PUBLIC -O2 -Wall -Werror -g)

if(DEFINED GENAVB_LIB_DIR)
  add_library(genavb SHARED IMPORTED)
  set_target_properties(genavb PROPERTIES IMPORTED_LOCATION "${GENAVB_LIB_DIR}/libgenavb.so")
endif()

target_link_libraries(${PROJECT_NAME} genavb)
target_link_libraries(${PROJECT_NAME} asound)
target_link_libraries(${PROJECT_NAME} m)

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
