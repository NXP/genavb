if(CONFIG_AVTP)

  genavb_include_os(${TARGET_OS}/avtp.cmake)

  genavb_add_library(NAME avtp
    SRCS
    avtp.c
    stream.c
    media_clock.c
    61883_iidc.c
    cvf.c
    acf.c
    aaf.c
    crf.c
    clock_domain.c
    clock_grid.c
    clock_source.c
    )

  genavb_link_libraries(TARGET ${avb} LIB avtp)

endif()
