if(CONFIG_MANAGEMENT)

  genavb_include_os(${TARGET_OS}/management.cmake)

  genavb_add_library(NAME management
    SRCS
    management.c
    mac_service.c
    )

  genavb_link_libraries(TARGET ${tsn} LIB management)

endif()
