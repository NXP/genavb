
genavb_target_add_srcs(TARGET genavb
  SRCS
  init.c
  control.c
  )

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET genavb
    SRCS
    streaming.c
    )
endif()

if(CONFIG_SOCKET)
  genavb_target_add_srcs(TARGET genavb
    SRCS
    socket.c
    )
endif()

genavb_target_add_linker_script(TARGET genavb LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/apis.map")
