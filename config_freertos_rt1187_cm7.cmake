if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS rtos)
set(TARGET_ARCH armv8m)

if(NOT DEFINED MCUX_SDK)
  message(WARNING "Undefined MCUX_SDK")
endif()

if(NOT DEFINED RTOS_APPS)
  message(WARNING "Undefined RTOS_APPS, default used")
endif()

if(NOT DEFINED RTOS_ABSTRACTION_LAYER_DIR)
  message(WARNING "Undefined RTOS_ABSTRACTION_LAYER_DIR")
endif()

if(EXISTS ${MCUX_SDK}/rtos/freertos/freertos_kernel)
set(RTOS_DIR ${MCUX_SDK}/rtos/freertos/freertos_kernel)
else()
set(RTOS_DIR ${MCUX_SDK}/rtos/freertos/freertos-kernel)
endif()
set(FREERTOS_PORT GCC/ARM_CM4F)
set(FREERTOS_CONFIG_INCLUDES ${RTOS_APPS}/boards/evkmimxrt1180a/demo_apps/avb_tsn/common/cm7 ${RTOS_APPS}/boards/src/demo_apps/avb_tsn/common)
if(NOT DEFINED APP_GENAVB_SDK_INCLUDE)
set(APP_GENAVB_SDK_INCLUDE ${RTOS_APPS}/boards/evkmimxrt1180a/demo_apps/avb_tsn/tsn_app/cm7 ${RTOS_APPS}/devices/MIMXRT118x/common/cm7)
endif()
set(MCUX_SDK_DEVICE_DIRS ${MCUX_SDK}/devices/MIMXRT1187 ${MCUX_SDK}/devices/MIMXRT1187/drivers ${MCUX_SDK}/devices/MIMXRT1187/drivers/cm7 ${MCUX_SDK}/platform/drivers/netc/socs/imxrt1180)
set(MCUX_SDK_TARGET_DIRS ${MCUX_SDK}/CMSIS/Core/Include)

set(INCLUDE_DIR usr/include)

add_compile_definitions(CPU_MIMXRT1187AVM8B_cm7)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-Os -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16)

add_compile_definitions(NET_RX_PACKETS=8)
