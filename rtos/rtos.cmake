# set os dependent target name
# for rtos there is no separate target for tsn, so ${tsn} equals ${avb}
set(avb stack-rtos)
set(tsn ${avb})

include_directories("${MCUX_SDK_TARGET_DIRS}")
include_directories("${APP_GENAVB_SDK_INCLUDE}")
include_directories("${MCUX_SDK_DEVICE_DIRS}")
include_directories("${MCUX_SDK}/components/phy")
include_directories(${MCUX_SDK_DRIVERS_DIRS})

genavb_add_os_library(NAME ${avb}
  SRCS
  assert.c
  avb_queue.c
  avtp.c
  clock.c
  debug_print.c
  fdb.c
  fqtss.c
  frame_preemption.c
  frer.c
  gpt.c
  gpt_rec.c
  gptp_dev.c
  hr_timer.c
  hsr.c
  hw_clock.c
  hw_timer.c
  ipc.c
  l2.c
  log.c
  mrp.c
  msgintr.c
  net.c
  net_bridge.c
  net_mdio.c
  net_logical_port.c
  net_phy.c
  net_port.c
  net_port_dsa.c
  net_port_enet.c
  net_port_enet_mdio.c
  net_port_enet_qos.c
  net_port_enet_qos_mdio.c
  net_port_enet_qos_stats.c
  net_port_enet_stats.c
  net_port_enetc_ep.c
  net_port_netc_1588.c
  net_port_netc_mdio.c
  net_port_netc_psfp.c
  net_port_netc_stats.c
  net_port_netc_stream_identification.c
  net_port_netc_sw.c
  net_port_netc_sw_dsa.c
  net_port_netc_sw_frer.c
  net_port_stats.c
  net_rx.c
  net_socket.c
  net_task.c
  net_tx.c
  pi.c
  prng_32.c
  psfp.c
  ptp.c
  rational.c
  scheduled_traffic.c
  stats_task.c
  stdlib.c
  stream_identification.c
  string.c
  timer.c
  tpm.c
  tpm_rec.c
  vlan.c
)

if(CONFIG_AVTP)
  genavb_target_add_srcs(TARGET ${avb}
    SRCS
    imx-pll.c
    media.c
    media_clock.c
    media_clock_gen_ptp.c
    media_clock_rec_pll.c
    media_queue.c
    mtimer.c
    )
endif()

set(RTOS_ABSTRACTION_LAYER_TARGET ${avb})
include(${RTOS_ABSTRACTION_LAYER_DIR}/rtos_abstraction_layer.cmake)
