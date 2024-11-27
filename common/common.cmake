
genavb_add_library(NAME common
  SRCS
  filter.c
  log.c
  managed_objects.c
  stats.c
  timer.c
  random.c
  fqtss.c
  )

genavb_target_add_srcs(TARGET gptp
  SRCS
  clock.c
  )

genavb_target_add_srcs(TARGET avtp
  SRCS
  61883_iidc.c
  aaf.c
  )

genavb_target_add_srcs(TARGET avdecc
  SRCS
  avdecc.c
  aaf.c
  hash.c
)

genavb_target_add_srcs(TARGET srp
  SRCS
  srp.c
  )

genavb_target_add_srcs(TARGET genavb
  SRCS
  61883_iidc.c
  aaf.c
  avdecc.c
  log.c
  srp.c
  )

genavb_link_libraries(TARGET ${avb} LIB common)
genavb_link_libraries(TARGET ${tsn} LIB common)
