if(CONFIG_SRP)

  genavb_include_os(${TARGET_OS}/srp.cmake)

  genavb_add_library(NAME srp
    SRCS
    srp.c
    mrp.c
    msrp_map.c
    msrp.c
    mvrp_map.c
    mvrp.c
    mmrp.c
    srp_managed_objects.c
    )

  genavb_link_libraries(TARGET ${tsn} LIB srp)

endif()
