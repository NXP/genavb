if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS rtos)
set(TARGET_ARCH armv7m)

if(NOT DEFINED MCUX_SDK)
  message(WARNING "Undefined MCUX_SDK")
endif()

if(NOT DEFINED RTOS_APPS)
  message(WARNING "Undefined RTOS_APPS, default used")
endif()

if(NOT DEFINED RTOS_ABSTRACTION_LAYER_DIR)
  message(WARNING "Undefined RTOS_ABSTRACTION_LAYER_DIR")
endif()

set(RTOS_DIR ${MCUX_SDK}/rtos/freertos/freertos-kernel)
set(FREERTOS_PORT GCC/ARM_CM4F)
set(FREERTOS_CONFIG_INCLUDES ${RTOS_APPS}/boards/evkmimxrt1170/demo_apps/avb_tsn/common ${RTOS_APPS}/boards/src/demo_apps/avb_tsn/common)
if(NOT DEFINED APP_GENAVB_SDK_INCLUDE)
set(APP_GENAVB_SDK_INCLUDE ${RTOS_APPS}/boards/evkmimxrt1170/demo_apps/avb_tsn/common)
endif()
set(MCUX_SDK_DEVICE_DIRS ${MCUX_SDK}/devices/MIMXRT1176 ${MCUX_SDK}/devices/MIMXRT1176/drivers ${MCUX_SDK}/devices/MIMXRT1176/drivers/cm7)
set(MCUX_SDK_TARGET_DIRS ${MCUX_SDK}/CMSIS/Core/Include)

set(INCLUDE_DIR usr/include)

add_compile_definitions(CPU_MIMXRT1176DVMAA_cm7 ENET_ENHANCEDBUFFERDESCRIPTOR_MODE)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-Os -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16)
# Disable the -Wmaybe-uninitialized warning that is generating false positives, which combined
# with -Werror causes compilation to fail. The false positives were first observed when using
# the -Os optimization flag and armgcc 13.2.
add_compile_options(-Wno-maybe-uninitialized)

add_compile_definitions(NET_RX_PACKETS=10)
