if(CONFIG_AVDECC)

  genavb_include_os(${TARGET_OS}/avdecc.cmake)

  genavb_add_library(NAME avdecc
    SRCS
    avdecc.c
    avdecc_ieee.c
    adp_milan.c
    adp.c
    adp_ieee.c
    aecp.c
    acmp.c
    acmp_ieee.c
    acmp_milan.c
    aem.c
    entity.c
    )

  genavb_link_libraries(TARGET ${avb} LIB avdecc)

endif()
