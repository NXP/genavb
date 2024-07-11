message("rtos_abstraction_layer (freertos) headers are included for target ${RTOS_ABSTRACTION_LAYER_TARGET}")

target_include_directories(${RTOS_ABSTRACTION_LAYER_TARGET} PRIVATE
  ${FREERTOS_CONFIG_INCLUDES}
  ${RTOS_DIR}/include
  ${RTOS_DIR}/portable/${FREERTOS_PORT}
  ${CMAKE_CURRENT_LIST_DIR}
)

include_guard(GLOBAL)
message("rtos_abstraction_layer (freertos) sources are included for target ${RTOS_ABSTRACTION_LAYER_TARGET}")

target_sources(${RTOS_ABSTRACTION_LAYER_TARGET} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/rtos_atomic.c
  ${CMAKE_CURRENT_LIST_DIR}/rtos_mqueue.c
  ${CMAKE_CURRENT_LIST_DIR}/rtos_sched.c
  ${CMAKE_CURRENT_LIST_DIR}/rtos_timer.c
  ${CMAKE_CURRENT_LIST_DIR}/rtos_thread.c
)
