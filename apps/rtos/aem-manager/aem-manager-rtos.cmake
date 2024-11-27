include(${CMAKE_CURRENT_LIST_DIR}/../../common/aem-manager/aem-manager-helpers.cmake)

message("aem-manager-rtos and common aem-manager-helpers headers are included for target ${AEM_MANAGER_RTOS_TARGET}")

target_include_directories(${AEM_MANAGER_RTOS_TARGET} PRIVATE
  ${AEM_MANAGER_HELPERS_HEADER_DIR}
  ${CMAKE_CURRENT_LIST_DIR}
)

message("aem-manager-rtos and common aem-manager-helpers sources are included for target ${AEM_MANAGER_RTOS_TARGET}")

target_sources(${AEM_MANAGER_RTOS_TARGET} PRIVATE
  ${AEM_MANAGER_HELPERS_SOURCES}
  ${CMAKE_CURRENT_LIST_DIR}/aem_manager_rtos.c
)
