option(BUILD_XDP_EBPF_PROGRAM "Rebuild ebpf program for XDP from source" OFF)

if(NOT DEFINED FIRMWARE_DIR)
  message(FATAL_ERROR "FIRMWARE_DIR not defined")
endif()

if(BUILD_XDP_EBPF_PROGRAM)
  message(STATUS "Rebuild of ebpf program for XDP from source enabled")

  find_program(LLC "llc" NO_CMAKE_FIND_ROOT_PATH)
  if(LLC STREQUAL "LLC-NOTFOUND")
    message(FATAL_ERROR "LLC not found")
  endif()

  find_program(CLANG "clang" NO_CMAKE_FIND_ROOT_PATH)
  if(CLANG STREQUAL "CLANG-NOTFOUND")
    message(FATAL_ERROR "CLANG not found")
  endif()

  find_program(LLVMLD "llvm-link" NO_CMAKE_FIND_ROOT_PATH)
  if(LLVMD STREQUAL "LLVMD-NOTFOUND")
    message(FATAL_ERROR "LLVMD not found")
  endif()

  set(GENAVB_INCLUDE ${CMAKE_SOURCE_DIR}/include)
  set(CXXFLAGS -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wunused -Wundef -Wold-style-definition -Wvla -Wshadow -Wdouble-promotion)
  set(CXXFLAGS ${CXXFLAGS} -I${GENAVB_INCLUDE} -I${GENAVB_INCLUDE}/linux)

  set(GENAVB_XDP ${CMAKE_BINARY_DIR}/ebpf/genavb-xdp.bin)
  set(GENAVB_XDP_IR ${CMAKE_BINARY_DIR}/ebpf/genavb-xdp.bc)
  set(OBJ ${CMAKE_BINARY_DIR}/ebpf/genavb_xdp_main.bc)
  set(SOURCE ${CMAKE_CURRENT_LIST_DIR}/genavb_xdp_main.c)

  add_custom_command(
    OUTPUT ${GENAVB_XDP}
    COMMAND ${LLC} ${GENAVB_XDP_IR} -march=bpf -filetype=obj -o ${GENAVB_XDP}
    DEPENDS ${GENAVB_XDP_IR}
  )

  add_custom_target(ebpf ALL DEPENDS ${GENAVB_XDP})

  add_custom_command(
    OUTPUT ${GENAVB_XDP_IR}
    COMMAND ${LLVMLD} ${OBJ} -o ${GENAVB_XDP_IR}
    DEPENDS ${OBJ}
    COMMENT ""
  )

  add_custom_command(
    OUTPUT ${OBJ}
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/ebpf
    COMMAND ${CLANG} -target arm64 ${CXXFLAGS} -O2 -emit-llvm -c ${SOURCE} -o ${OBJ}
    DEPENDS ${SOURCE}
    COMMENT ""
  )

  install(FILES ${GENAVB_XDP} DESTINATION ${FIRMWARE_DIR})
else()
  message(STATUS "Rebuild of ebpf program for XDP from source disabled, install pre-compiled binary")
  install(FILES ${CMAKE_SOURCE_DIR}/linux/firmware/genavb-xdp.bin DESTINATION ${FIRMWARE_DIR})
endif()
