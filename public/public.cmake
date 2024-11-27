genavb_conditional_include(${TARGET_OS}/public.cmake)

genavb_target_add_srcs(TARGET ${avb}
  SRCS
  aem_helpers.c
  sr_class.c
  qos.c
  helpers.c
  )

genavb_target_add_srcs(TARGET ${tsn}
  SRCS
  qos.c
  helpers.c
  sr_class.c
  )

genavb_target_add_srcs(TARGET genavb
  SRCS
  sr_class.c
  qos.c
  helpers.c
  )
