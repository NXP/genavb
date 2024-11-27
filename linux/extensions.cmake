
# genavb_add_executable(NAME <target> SRCS <src1 src2 ...>)
function(genavb_add_executable)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_executable(${ARG_NAME} ${srcs})

  add_dependencies(stack ${ARG_NAME})

  genavb_add_os_component_defines(${ARG_NAME})

  target_link_libraries(${ARG_NAME} PRIVATE m)
  target_link_libraries(${ARG_NAME} PRIVATE pthread)

  set_target_properties(${ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

  install(TARGETS ${ARG_NAME} DESTINATION ${BIN_DIR})
endfunction()

function(genavb_add_executable_alias exec_name alias_name)
  install(CODE "execute_process(COMMAND ln -rsf \$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/${BIN_DIR}/${exec_name} \$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/${BIN_DIR}/${alias_name})")
endfunction()

# genavb_add_shared_library(NAME <target> SRCS <src1 src2 ...>)
function(genavb_add_shared_library)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_library(${ARG_NAME} SHARED ${srcs})

  add_dependencies(stack ${ARG_NAME})

  target_link_libraries(${ARG_NAME} PRIVATE -Wl,--no-undefined)

  genavb_add_os_component_defines(${ARG_NAME})

  set_target_properties(${ARG_NAME} PROPERTIES VERSION 1.0)
  set_target_properties(${ARG_NAME} PROPERTIES SOVERSION 1)

  set_target_properties(${ARG_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

  install(TARGETS ${ARG_NAME} DESTINATION ${LIB_DIR})
endfunction()

# genavb_add_dependencies(TARGET <target> DEP <target-dependency ...>)
function(genavb_add_dependencies)
  cmake_parse_arguments(ARG "" "TARGET" "DEP" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()
  add_dependencies(${ARG_TARGET} ${ARG_DEP})
endfunction()

function(genavb_generate_archives)
  add_custom_target(stack-core
    DEPENDS ${genavb_used_libs}
    )
endfunction()

function(genavb_install)
  set(common_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/genavb/*.h")
  set(os_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/${TARGET_OS}/os/*.h")

  # prepare include dir for applications
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/include/genavb/os
    COMMAND cp -a ${common_headers} ${CMAKE_CURRENT_BINARY_DIR}/include/genavb
    COMMAND cp -a ${os_headers} ${CMAKE_CURRENT_BINARY_DIR}/include/genavb/os
    COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/include
    DEPENDS ${common_headers} ${os_headers}
    COMMENT "Generate includes"
  )

  add_custom_target(include-prep ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include
  )

  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ DESTINATION ${INCLUDE_DIR})
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/ DESTINATION ${BIN_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/ DESTINATION ${LIB_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endfunction()
