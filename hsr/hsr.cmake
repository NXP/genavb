if(CONFIG_HSR)

	genavb_include_os(${TARGET_OS}/hsr.cmake)

	genavb_target_add_srcs(TARGET ${avb} SRCS hsr.c)

endif()
