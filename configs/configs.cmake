function(genavb_set_option option value)
  # Define in parent scope to be visible to parent CMakeList
  set(${option} ${value} PARENT_SCOPE)
  # Define in current (function) scope to be used here.
  set(${option} ${value})

  if(${option})
    add_definitions(-D${option}=1)
  endif()
endfunction()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CONFIG}.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/${CONFIG}.cmake)
else()
  message(FATAL_ERROR "Configuration file for ${CONFIG} does not exist")
endif()

message(STATUS "CONFIG: ${CONFIG}")
