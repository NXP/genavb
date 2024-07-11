cmake_minimum_required(VERSION 3.10)

project(tsn-app)

include_directories(${GENAVB_INCLUDE_DIR})

add_executable(${PROJECT_NAME}
  main.c
  tsn_task.c
  tsn_tasks_config.c
  thread_config.c
  cyclic_task.c
  serial_controller.c
  network_only.c
  tsn_timer.c
  ../common/stats.c
  ../common/stats.c
  ../common/thread.c
  ../common/log.c
  ../common/time.c
  ../common/timer.c
)

set(OPCUA_SUPPORT $ENV{OPCUA_SUPPORT})

if(OPCUA_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC OPCUA_SUPPORT)
  target_sources(${PROJECT_NAME} PRIVATE opcua/opcua_server.c opcua/model/tsn_app_model.c)
  target_link_libraries(${PROJECT_NAME} open62541)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -O2 -Wall -Werror -g -DSTATS_LOG)
target_link_libraries(${PROJECT_NAME} -Wl,-unresolved-symbols=ignore-in-shared-libs)

if(DEFINED GENAVB_LIB_DIR)
  add_library(genavb SHARED IMPORTED)
  set_target_properties(genavb PROPERTIES IMPORTED_LOCATION "${GENAVB_LIB_DIR}/libgenavb.so")
endif()

target_link_libraries(${PROJECT_NAME} genavb)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} dl)

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
