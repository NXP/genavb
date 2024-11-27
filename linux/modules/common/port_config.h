/*
 * Copyright 2018-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _PORT_CONFIG_H_
#define _PORT_CONFIG_H_

#ifdef __KERNEL__
#include <linux/if.h>
#include "genavb/config.h"
#else
#include <net/if.h>
#define IFNAMSIZ IF_NAMESIZE
#endif

enum logical_port_type {
	LOGICAL_PORT_TYPE_PHYSICAL,
	LOGICAL_PORT_TYPE_BRIDGE
};


struct logical_port_config {
	unsigned int type;
	char name[IFNAMSIZ];	/* Linux interface name */
	bool enabled;
	bool is_hybrid;
	union {
		struct {
			unsigned int id;
			unsigned int port;
		} bridge;

		struct {
			unsigned int id;
		} physical;
	};
};

#ifdef __KERNEL__

#if !defined(CFG_ENDPOINT_WITH_SWITCH)
/* 1:1 mapping between logical and physical ports. */
#define CFG_LOGICAL_NUM_PORT	2
#define CFG_PHYSICAL_NUM_PORT	2
#define CFG_LOGICAL_PORT_MAX_PER_PHYSICAL  1
#else
/* mapping of two logical ports to the same physical interface. */
#define CFG_LOGICAL_NUM_PORT	2
#define CFG_PHYSICAL_NUM_PORT	1
#define CFG_LOGICAL_PORT_MAX_PER_PHYSICAL  2
#endif

#if CFG_LOGICAL_PORT_MAX_PER_PHYSICAL * CFG_PHYSICAL_NUM_PORT > CFG_LOGICAL_NUM_PORT
#error wrong mapping of logical to physical ports
#endif

/* When no port information is available, redirect rx/tx packets on physical port to the first logical port mapped to it. */
#define CFG_DEFAULT_LOGICAL_PORT_MAP_INDEX	0

/* CFG_PORTS maps directly to the number of supported physical ports (Endpoint MAC controllers) */
#define CFG_PORTS	CFG_PHYSICAL_NUM_PORT

/* Currently, the kernel module supports only endpoint mode: logical ports map to number of supported endpoints */
#define CFG_ENDPOINT_NUM	(CFG_LOGICAL_NUM_PORT)

#if CFG_ENDPOINT_NUM > CFG_MAX_ENDPOINTS
#error Too many endpoint interfaces defined
#endif

struct physical_port_config {
	char name[IFNAMSIZ];
	unsigned int num_logical_ports;
	unsigned int logical_ports[CFG_LOGICAL_PORT_MAX_PER_PHYSICAL];
};

#if !defined(CFG_ENDPOINT_WITH_SWITCH)

/* 1:1 mapping: Two physical ports mapped to two logical ports */
#define LOGICAL_PORT_CONFIG	\
{	\
	[0] = {	\
		.type = LOGICAL_PORT_TYPE_PHYSICAL,	\
		.name = "eth0",	\
		.enabled = true,	\
		.physical.id = 0,	\
	},	\
	[1] = {	\
		.type = LOGICAL_PORT_TYPE_PHYSICAL,	\
		.name = "eth1",	\
		.enabled = true,	\
		.physical.id = 1,	\
	},	\
}

#define PHYSICAL_PORT_CONFIG	\
{	\
	[0] = {	\
		.name = "eth0",	        \
		.num_logical_ports = 1,	\
		.logical_ports = {0},   \
	},	\
	[1] = {	\
		.name = "eth1",	        \
		.num_logical_ports = 1,	\
		.logical_ports = {1},   \
	},	\
}

#else /* !defined(CFG_ENDPOINT_WITH_SWITCH)*/

/* One physical port mapped to two logical ports*/
#define LOGICAL_PORT_CONFIG	\
{	\
	[0] = {	\
		.type = LOGICAL_PORT_TYPE_PHYSICAL,	\
		.name = "swp0",	\
		.enabled = true,	\
		.physical.id = 0,	\
	},	\
	[1] = {	\
		.type = LOGICAL_PORT_TYPE_PHYSICAL,	\
		.name = "swp1",	\
		.enabled = true,	\
		.physical.id = 0,	\
	},	\
}

#define PHYSICAL_PORT_CONFIG	\
{	\
	[0] = {	\
		.name = "eth0",	        \
		.num_logical_ports = 2,	\
		.logical_ports = {0, 1},\
	},	\
}

#endif /* !defined(CFG_ENDPOINT_WITH_SWITCH)*/
#endif /* __KERNEL__ */
#endif /* _PORT_CONFIG_H_ */
