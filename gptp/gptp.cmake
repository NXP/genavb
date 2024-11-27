if (CONFIG_GPTP)

  genavb_include_os(${TARGET_OS}/gptp.cmake)

  genavb_add_library(NAME gptp
    SRCS
    gptp.c
    md_fsm_802_3.c
    port_fsm.c
    clock_sl_fsm.c
    target_clock_adj.c
    bmca.c
    site_fsm.c
    clock_ms_fsm.c
    ptp_time_ops.c
    gptp_managed_objects.c
    )

  genavb_link_libraries(TARGET ${tsn} LIB gptp)

endif ()
