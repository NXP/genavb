genavb_target_add_srcs(TARGET ${avb}
  SRCS
  control.c
  fdb.c
  frame_preemption.c
  frer.c
  generic.c
  init.c
  psfp.c
  qos.c
  scheduled_traffic.c
  stream_identification.c
  timer.c
  vlan.c
  ../error.c
  ../control.c
  ../clock.c
  )

if(CONFIG_SOCKET)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    socket.c
    ../socket.c
    )
endif()

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    streaming.c
    ../streaming.c
    )
endif()

if(CONFIG_HSR)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    hsr.c
    )
endif()

if(CONFIG_DSA)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    dsa.c
    )
endif()
