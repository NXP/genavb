include(${CMAKE_CURRENT_LIST_DIR}/configs/configs.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/scripts/scripts.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ebpf/ebpf.cmake)

if(CONFIG_AVTP OR CONFIG_AVDECC OR CONFIG_MAAP)
  set(avb avb)
endif()

if(CONFIG_GPTP OR CONFIG_SRP OR CONFIG_MANAGEMENT)
  set(tsn tsn)
endif()

# Include modules
include_directories("${CMAKE_BINARY_DIR}")

genavb_add_executable(NAME ${avb}
  SRCS
  assert.c
  stdlib.c
  string.c
  avb_main.c
  net.c
  log.c
  timer.c
  ipc.c
  clock.c
  cfgfile.c
  epoll.c
  init.c
  os_config.c
  net_logical_port.c
  )

set(genavb genavb)

genavb_add_shared_library(NAME ${genavb}
  SRCS
  ipc.c
  log.c
  clock.c
  string.c
  stdlib.c
  epoll.c
  init.c
  assert.c
  cfgfile.c
  os_config.c
  net_logical_port.c
)

genavb_add_executable(NAME ${tsn}
  SRCS
  fdb.c
  tsn_main.c
  stdlib.c
  string.c
  net.c
  log.c
  timer.c
  clock.c
  cfgfile.c
  epoll.c
  ipc.c
  init.c
  assert.c
  os_config.c
  net_logical_port.c
  vlan.c
)

# Backward compatibility
genavb_add_executable_alias("${tsn}" "fgptp")

genavb_add_dependencies(TARGET ${avb} DEP modules-dir)
genavb_add_dependencies(TARGET ${tsn} DEP modules-dir)
genavb_add_dependencies(TARGET genavb DEP modules-dir)

# net_ipc files for tsn process
genavb_target_add_srcs(TARGET ${tsn} SRCS net_ipc.c)
# net_std files for tsn process
genavb_target_add_srcs(TARGET ${tsn} SRCS net_std.c net_std_socket_filters.c rtnetlink.c fqtss.c fqtss_std.c fdb_std.c)
# net_avb files for avb and tsn processes
genavb_target_add_srcs(TARGET ${avb} SRCS net_avb.c shmem.c)
genavb_target_add_srcs(TARGET ${tsn} SRCS net_avb.c shmem.c fqtss.c fqtss_avb.c)

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET ${avb} SRCS media_clock.c media.c timer_media.c)
  genavb_target_add_srcs(TARGET ${tsn} SRCS timer_media.c)
endif()

if(CONFIG_SOCKET)
  include(CheckSymbolExists)
  include(CheckIncludeFile)

  # net_std for genavb shared lib
  genavb_target_add_srcs(TARGET genavb SRCS net.c net_std.c net_std_socket_filters.c)

  # Check that we have libbpf headers from sysroot
  unset(HAVE_LIBBPF_HEADERS CACHE)
  check_include_file("bpf/libbpf.h" HAVE_LIBBPF_HEADERS)
  if(NOT HAVE_LIBBPF_HEADERS)
    message(WARNING "Cannot detect libbpf headers, genavb shared library will not include AF_XDP support.")
  endif()

  # Check that we have libxdp headers from sysroot
  unset(HAVE_LIBXDP_HEADERS CACHE)
  check_include_file("xdp/libxdp.h" HAVE_LIBXDP_HEADERS)
  if(NOT HAVE_LIBXDP_HEADERS)
    message(WARNING "Cannot detect libxdp headers, genavb shared library will not include AF_XDP support.")
  endif()

  # Toolchain needs to have recent if_xdp.h kernel header (>= 5.4)
  unset(HAVE_XDP_KERNEL_HEADERS CACHE)
  check_symbol_exists(XDP_RING_NEED_WAKEUP linux/if_xdp.h HAVE_XDP_KERNEL_HEADERS)
  if(NOT DEFINED HAVE_XDP_KERNEL_HEADERS)
    message(WARNING "Cannot detect recent if_xdp.h kernel header, genavb shared library will not include AF_XDP support.")
  endif()

  if (HAVE_LIBBPF_HEADERS AND HAVE_XDP_KERNEL_HEADERS AND HAVE_LIBXDP_HEADERS)
    # Add net_xdp for genavb shared lib
    genavb_target_add_srcs(TARGET genavb SRCS net_xdp.c pool.c)

    # For compatibility with glibc older than v2.34, link to libdl
    target_link_libraries(genavb PRIVATE dl)
  endif()
endif()
