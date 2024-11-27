if(CONFIG_MAAP)

  genavb_conditional_include(${TARGET_OS}/maap.cmake)

  genavb_add_library(NAME maap
    SRCS
    maap.c
    )

  genavb_link_libraries(TARGET ${avb} LIB maap)

endif()
