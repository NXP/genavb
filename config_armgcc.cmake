# TOOLCHAIN EXTENSION
if(WIN32)
    set(toolchain_ext ".exe")
else()
    set(toolchain_ext "")
endif()

set(toolchain_dir $ENV{ARMGCC_DIR})
string(REGEX REPLACE "\\\\" "/" toolchain_dir "${toolchain_dir}")

if(NOT toolchain_dir)
    message(FATAL_ERROR "*** Please set ARMGCC_DIR in environment variables ***")
endif()

message(STATUS "toolchain_dir: " ${toolchain_dir})

set(target_prefix $ENV{TARGET_PREFIX})
if(NOT target_prefix)
    set(target_prefix "arm-none-eabi-")
    message(WARNING "*** TARGET_PREFIX environment variable not found, defaults to ${target_prefix} ***")
endif()

set(toolchain_bin_dir ${toolchain_dir}/bin)
set(toolchain_inc_dir ${toolchain_dir}/${target_prefix}/include)
set(toolchain_lib_dir ${toolchain_dir}/${target_prefix}/lib)

if(target_prefix MATCHES "linux")
    set(CMAKE_SYSTEM_NAME Linux)
else()
    set(CMAKE_SYSTEM_NAME Generic)
endif()
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER ${toolchain_bin_dir}/${target_prefix}gcc${toolchain_ext})
set(CMAKE_CXX_COMPILER ${toolchain_bin_dir}/${target_prefix}g++${toolchain_ext})
set(CMAKE_ASM_COMPILER ${toolchain_bin_dir}/${target_prefix}gcc${toolchain_ext})

set(CMAKE_FIND_ROOT_PATH ${toolchain_dir}/${target_prefix} ${extra_find_path})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
