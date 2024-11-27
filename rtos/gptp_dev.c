/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GPTP event generation
 @details
*/

#include "gptp_dev.h"
#include "clock.h"
#include "common/log.h"
#include "net_port_enet.h"
#include "net_port_enet_qos.h"

#if CFG_NUM_ENET_MAC
#include "fsl_enet.h"
#endif
#if CFG_NUM_ENET_QOS_MAC
#include "fsl_enet_qos.h"
#endif

static struct gptp_dev gptp_devices[GPTP_MAX_DEVICES] = {
	[0] = {
		.net_port = 0,
		.port = NULL,
		.enabled = false,
	},
};

#if CFG_NUM_ENET_MAC || CFG_NUM_ENET_QOS_MAC
static int gptp_to_local(struct net_port *port, uint32_t gptp_ts, uint64_t *local_cycles)
{
	uint64_t time;
	uint32_t ls_nanosec;

	if (os_clock_gettime64(port->clock[PORT_CLOCK_GPTP_0], &time) < 0)
		goto err;

	ls_nanosec = time & 0x00000000FFFFFFFF;

	if (ls_nanosec < gptp_ts)
		time = (time & 0xFFFFFFFF00000000);
	else
		time = (time & 0xFFFFFFFF00000000) + 0x100000000;

	time |= gptp_ts;

	if (clock_time_to_cycles(port->clock[PORT_CLOCK_GPTP_0], time, local_cycles) < 0)
		goto err;

	return 0;

err:
	return -1;
}
#endif

static struct gptp_dev *__gptp_get_device(uint32_t index)
{
	if (index > GPTP_MAX_DEVICES)
		return NULL;

	return &gptp_devices[index];
}

#if CFG_NUM_ENET_MAC
static int gptp_enet_event_start(struct net_port *port, uint32_t ts_0, uint32_t ts_1)
{
	uint64_t cycles_0, cycles_1;

	if (gptp_to_local(port, ts_0, &cycles_0) < 0)
		goto err;

	if (gptp_to_local(port, ts_1, &cycles_1) < 0)
		goto err;

	ENET_Ptp1588SetChannelCmpValue((ENET_Type *)port->base, port->timer_event.channel, cycles_0);
	ENET_Ptp1588SetChannelMode((ENET_Type *)port->base, port->timer_event.channel, kENET_PtpChannelToggleCompare, false);
	ENET_Ptp1588SetChannelCmpValue((ENET_Type *)port->base, port->timer_event.channel, cycles_1);

	return 0;

err:
	return -1;
}

static void gptp_enet_event_stop(struct net_port *port)
{
	ENET_Ptp1588SetChannelCmpValue((ENET_Type *)port->base, port->timer_event.channel, 0);
	ENET_Ptp1588ClearChannelStatus((ENET_Type *)port->base, port->timer_event.channel);
}

static int gptp_enet_event_reload(struct net_port *port, uint32_t ts)
{
	uint64_t cycles;

	if (gptp_to_local(port, ts, &cycles) < 0)
		return -1;

	ENET_Ptp1588SetChannelCmpValue((ENET_Type *)port->base, port->timer_event.channel, cycles);
	ENET_Ptp1588ClearChannelStatus((ENET_Type *)port->base, port->timer_event.channel);

	return 0;
}

__init static int gptp_enet_event_init(struct net_port *port)
{
	return 0;
}

__exit static void gptp_enet_event_exit(struct net_port *port)
{
}
#else
static int gptp_enet_event_start(struct net_port *port, uint32_t ts_0, uint32_t ts_1) { return -1; }
static void gptp_enet_event_stop(struct net_port *port) { }
static int gptp_enet_event_reload(struct net_port *port, uint32_t ts) { return -1; }
__init static int gptp_enet_event_init(struct net_port *port) { return -1; }
__exit static void gptp_enet_event_exit(struct net_port *port) { }
#endif /* CFG_NUM_ENET_MAC */

#if CFG_NUM_ENET_QOS_MAC
static int gptp_enet_qos_event_start(struct net_port *port, uint32_t ts_0, uint32_t ts_1)
{
	uint64_t seconds;
	uint32_t nanoseconds;
	uint64_t local_cycles;

	if (gptp_to_local(port, ts_1, &local_cycles) < 0)
		goto err;

	seconds = local_cycles / NSECS_PER_SEC;
	nanoseconds = local_cycles % NSECS_PER_SEC;

	if (ENET_QOS_Ptp1588PpsSetTrgtTime((ENET_QOS_Type *)port->base, port->timer_event.channel, seconds, nanoseconds) != kStatus_Success)
		goto err;

	ENET_QOS_Ptp1588PpsSetWidth((ENET_QOS_Type *)port->base, port->timer_event.channel, 0x0000000f);
	ENET_QOS_Ptp1588PpsSetInterval((ENET_QOS_Type *)port->base, port->timer_event.channel, 0x0000001f);

	if (ENET_Ptp1588PpsControl((ENET_QOS_Type *)port->base, port->timer_event.channel, kENET_QOS_PtpPpsTrgtModeOnlySt, kENET_QOS_PtpPpsCmdSSP) != kStatus_Success)
		goto err;

	return 0;

err:
	return -1;
}

static void gptp_enet_qos_event_stop(struct net_port *port)
{
	ENET_QOS_Ptp1588PpsSetTrgtTime((ENET_QOS_Type *)port->base, port->timer_event.channel, 0, 0);
	ENET_Ptp1588PpsControl((ENET_QOS_Type *)port->base, port->timer_event.channel, kENET_QOS_PtpPpsTrgtModeOnlySt, kENET_QOS_PtpPpsCmdCS);
}

static int gptp_enet_qos_event_reload(struct net_port *port, uint32_t ts)
{
	uint64_t seconds;
	uint32_t nanoseconds;
	uint64_t local_cycles;

	if (gptp_to_local(port, ts, &local_cycles) < 0)
		goto err;

	seconds = local_cycles / NSECS_PER_SEC;
	nanoseconds = local_cycles % NSECS_PER_SEC;

	if (ENET_QOS_Ptp1588PpsSetTrgtTime((ENET_QOS_Type *)port->base, port->timer_event.channel, seconds, nanoseconds) != kStatus_Success)
		goto err;

	if (ENET_Ptp1588PpsControl((ENET_QOS_Type *)port->base, port->timer_event.channel, kENET_QOS_PtpPpsTrgtModeOnlySt, kENET_QOS_PtpPpsCmdSSP) != kStatus_Success)
		goto err;

	return 0;

err:
	return -1;
}

__init static int gptp_enet_qos_event_init(struct net_port *port)
{
	return 0;
}

__exit static void gptp_enet_qos_event_exit(struct net_port *port)
{
}
#else
static int gptp_enet_qos_event_start(struct net_port *port, uint32_t ts_0, uint32_t ts_1) { return -1; }
static void gptp_enet_qos_event_stop(struct net_port *port) { }
static int gptp_enet_qos_event_reload(struct net_port *port, uint32_t ts) { return -1; }
__init static int gptp_enet_qos_event_init(struct net_port *port) { return -1; }
__exit static void gptp_enet_qos_event_exit(struct net_port *port) { }
#endif /* CFG_NUM_ENET_QOS_MAC */

int gptp_event_start(struct gptp_dev *dev, uint32_t ts_0, uint32_t ts_1)
{
	int rc;

	if (dev->enabled) {
		os_log(LOG_ERR, "gptp device already enabled\n");
		rc = -1;
		goto err;
	}

	rc = dev->drv_ops.start(dev->port, ts_0, ts_1);
	if (rc)
		goto err;

	dev->enabled = true;

err:
	return rc;
}

void gptp_stop(struct gptp_dev *dev)
{
	dev->drv_ops.stop(dev->port);
	dev->enabled = false;
}

int gptp_tc_reload(struct gptp_dev *dev, uint32_t ts)
{
	int rc;

	if (!dev->port) {
		os_log(LOG_ERR, "net port not initialized\n");
		rc = -1;
		goto err;
	}

	rc = dev->drv_ops.reload(dev->port, ts);

err:
	return rc;
}

__init struct gptp_dev *gptp_event_init(uint32_t gptp_dev_index)
{
	struct gptp_dev *dev;

	if ((dev = __gptp_get_device(gptp_dev_index)) == NULL) {
		os_log(LOG_ERR, "invalid gptp device index\n");
		goto err;
	}

	dev->port = &ports[dev->net_port];

	switch (dev->port->drv_type) {
	case ENET_t:
	case ENET_1G_t:
		dev->drv_ops.start = gptp_enet_event_start;
		dev->drv_ops.stop = gptp_enet_event_stop;
		dev->drv_ops.reload = gptp_enet_event_reload;
		dev->drv_ops.init = gptp_enet_event_init;
		dev->drv_ops.exit = gptp_enet_event_exit;
		break;
	case ENET_QOS_t:
		dev->drv_ops.start = gptp_enet_qos_event_start;
		dev->drv_ops.stop = gptp_enet_qos_event_stop;
		dev->drv_ops.reload = gptp_enet_qos_event_reload;
		dev->drv_ops.init = gptp_enet_qos_event_init;
		dev->drv_ops.exit = gptp_enet_qos_event_exit;
		break;
	case ENETC_1G_t:
	case ENETC_PSEUDO_1G_t:
	case NETC_SW_t:
	default:
		os_log(LOG_ERR, "Driver type not supported\n");
		goto err;
	}

	if (dev->drv_ops.init(dev->port) < 0) {
		os_log(LOG_ERR, "Driver init failed\n");
		goto err;
	}

	return dev;

err:
	return NULL;
}

__exit void gptp_event_exit(struct gptp_dev *dev)
{
	dev->drv_ops.exit(dev->port);
}
