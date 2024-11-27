/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public initialization API
 \details initialization API definition for the GenAVB/TSN library

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_INIT_API_H_
#define _GENAVB_PUBLIC_INIT_API_H_

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "config.h"

/**
 * \defgroup init 			Initialization
 * \ingroup library
 *
 * \if LINUX
 *
 * The initialization API's allow to initialize the GenAVB/TSN stack library. To initialize the library call ::genavb_init, to exit ::genavb_exit. The handle returned by ::genavb_init is
 * required to call most other API's.
 *
 * \else
 *
 * The initialization API's allow to initialize the GenAVB/TSN stack. To initialize the stack call ::genavb_init, to exit ::genavb_exit. The handle returned by ::genavb_init is
 * required to call most other API's.
 *
 * It's possible to change the default stack configuration, before calling ::genavb_init. To retrieve the default configuration use ::genavb_get_default_config (with a genavb_config structure allocated in the stack or heap). Then, override the required settings in genavb_config and call ::genavb_set_config. After calling ::genavb_init, genavb_config structure may be discarded.
 *
 * \endif
 */

#define AVDECC_FAST_CONNECT_MODE		(1 << 0) /* Global fast-connect mode (includes saving a connection state) */
#define AVDECC_FAST_CONNECT_BTB			(1 << 1) /* Global back to back mode, enables connecting to the first talker appearing on the network (demo purpose) */

/* Runtime configurations */
#define ACMP_CFG_MAX_UNIQUE_ID		8

#define AVDECC_WAITMASK_MEDIA_STACK	(1 << 0)
#define AVDECC_WAITMASK_CONTROLLER	(1 << 1)
#define AVDECC_WAITMASK_CONTROLLED	(1 << 2)

/**
 * \cond RTOS
 */

/**
 * \ingroup init
 * AVDECC entity configuration
 */
struct avdecc_entity_config {
	unsigned int flags;
	unsigned int channel_waitmask;
	uint64_t entity_id;
	uint64_t association_id;				/* Association ID to use for the entity */
	/* Fast-connect remote information */
	uint64_t talker_entity_id[ACMP_CFG_MAX_UNIQUE_ID];
	unsigned int talker_entity_id_n;
	uint16_t talker_unique_id[ACMP_CFG_MAX_UNIQUE_ID];
	unsigned int talker_unique_id_n;
	uint16_t listener_unique_id[ACMP_CFG_MAX_UNIQUE_ID];
	unsigned int listener_unique_id_n;
	unsigned int valid_time;
	unsigned int max_listener_streams;
	unsigned int max_talker_streams;
	unsigned int max_listener_pairs;
	unsigned int max_inflights;
	unsigned int max_unsolicited_registrations;
	unsigned int max_ptlv_entries; /**< Maximum number of tracked clock ids in the path trace */
	bool milan_mode;
	void *aem;
};

/**
 * \ingroup init
 * AVDECC stack component configuration
 */
struct avdecc_config {
	genavb_log_level_t log_level;		/**< AVDECC log level */
	bool enabled;
	bool srp_enabled;
	bool management_enabled;
	bool milan_mode;
	bool use_gptp_bridge_stack;
	unsigned int num_entities;
	unsigned int max_entities_discovery;  /**< Maximum number of discoverable entities */
	struct avdecc_entity_config entity_cfg[CFG_AVDECC_NUM_ENTITIES];
	unsigned int port_max;
	unsigned int logical_port_list[CFG_MAX_NUM_PORT];
};

/**
 * \ingroup init
 * AVTP stack component configuration
 */
struct avtp_config {
	genavb_log_level_t log_level;		/**< AVTP log level */
	unsigned int port_max;
	unsigned int logical_port_list[CFG_MAX_NUM_PORT];
	unsigned int clock_gptp_list[CFG_MAX_NUM_PORT];
};

/**
 * \ingroup init
 * MSRP configuration flag.
 */
#define MSRP_SEND_LEAVE_ON_DECLARATION_UPDATE   (1 << 0) /**< Send a Leave on MSRP talker/listener attribute declaration change, before sending a Join */

/**
 * \ingroup init
 * SRP stack component configuration
 */
struct srp_config {
	genavb_log_level_t log_level;		/**< SRP log level */

	unsigned int is_bridge;

	unsigned int port_max;
	unsigned int logical_port_list[CFG_MAX_NUM_PORT];

	unsigned int management_enabled;

	struct mrp_config {
		unsigned int leave_timeout;
	} mrp_cfg;

	struct msrp_config {
		unsigned int is_bridge;
		unsigned int flags;
		unsigned int port_max;
		unsigned int logical_port_list[CFG_MAX_NUM_PORT];
		unsigned int enabled;
	} msrp_cfg;

	struct mvrp_config {
		unsigned int is_bridge;
		unsigned int port_max;
		unsigned int logical_port_list[CFG_MAX_NUM_PORT];
	} mvrp_cfg;
};

/**
 * \ingroup init
 * Management stack component configuration
 */
struct management_config {
	genavb_log_level_t log_level;		/**< Management log level */
	unsigned int is_bridge;
	unsigned int port_max;
	unsigned int logical_port_list[CFG_MAX_NUM_PORT];
};

/**
 * \ingroup init
 * GPTP port configuration
 */
struct fgptp_port_config {
	/* For domain 0 only */
	uint8_t portRole;
	uint8_t ptpPortEnabled;
	int rxDelayCompensation;
	int txDelayCompensation;

	/* Timing params */
	int8_t initialLogPdelayReqInterval;
	int8_t initialLogSyncInterval;
	int8_t initialLogAnnounceInterval;

	/*Avnu AutoCDSFunctionalSpec-1_4 - 6.2.1.5 / 6.2.1.6 */
	int8_t operLogPdelayReqInterval; /* A device moves to this value on all slave ports once the measured values have stabilized*/
	int8_t operLogSyncInterval; /*operLogSyncInterval is the Sync interval that a device moves to and signals on a slave port once it has achieved synchronization*/

	/* For all domains */
	uint8_t delayMechanism[CFG_MAX_GPTP_DOMAINS];
	uint8_t allowedLostResponses;
	uint8_t allowedFaults;
};

/**
 * \ingroup init
 * GPTP domain configuration
 */
struct fgptp_domain_config {
	/* General params */
	int domain_number;

	unsigned int clock_target;
	unsigned int clock_source;

	/* Grandmaster params */
	uint8_t gmCapable;	/* set to 1 if this device is grandmaster capable */
	uint8_t priority1;
	uint8_t priority2;
	uint8_t clockClass;
	uint8_t clockAccuracy;
	uint16_t offsetScaledLogVariance;
};

/**
 * \ingroup init
 * GPTP stack component configuration
 */
struct fgptp_config {
	/* General params */
	genavb_log_level_t log_level;		/**< gPTP log level */

	unsigned int is_bridge;

	unsigned int profile; /* gptp profile (standard, automotive) */

	unsigned int domain_max;

	unsigned int port_max;

	unsigned int logical_port_list[CFG_MAX_NUM_PORT];

	unsigned int management_enabled;

	unsigned int clock_local;

	uint64_t gm_id; /* grand master id used in case of static slave configuration or resulting from bmca */
	uint64_t neighborPropDelayThreshold; /*expressed in ns. The propagation time threshold, above which a port is not considered capable of participating in the IEEE 802.1AS protocol.*/
	unsigned int rsync; /* set to 1 to enable rsync feature */
	unsigned int rsync_interval; /* defines rsync packet interval in ms*/
	unsigned int statsInterval; /* defines the interval in second between statistics output */

	/* Automotive profile params */
	uint8_t neighborPropDelay_mode;					/* set to 1 if predefined pdelay mechanism is used. 0 means relying on standard pdelay exchange */
	double initial_neighborPropDelay[CFG_MAX_NUM_PORT];	/* expressed in ns. per port predefined value, but filled using single value from the default configuration file */
	double neighborPropDelay_sensitivity;			/* expressed in ns. global to all ports. defined the amount of ns between to pdelay measurement that would trigger a new indication to the host main code */

	void (*sync_indication)();
	void (*gm_indication)();
	void (*pdelay_indication)();

	/* per port settings */
	struct fgptp_port_config port_cfg[CFG_MAX_NUM_PORT];

	/* per domain settings */
	struct fgptp_domain_config domain_cfg[CFG_MAX_GPTP_DOMAINS];

	/* IEEE 802.1AS-2011 operation */
	unsigned int force_2011;	/* set to 1 if the stask operates per IEEE 802.1AS-2011 standard (no multi domains support) */
};

/*
 * \ingroup init
 * MAAP domain configuration
 */
struct maap_config {
	/* General parameters */
	genavb_log_level_t log_level;		/**< MAAP log level */

	unsigned int port_max;
	bool management_enabled;

	unsigned int logical_port_list[CFG_MAX_NUM_PORT];
};

typedef enum {
	HSR_UNUSED_PORT,
	HSR_HOST_PORT,
	HSR_RING_PORT,
	HSR_QUARD_RING_PORT,
	HSR_EXTERNAL_PORT,
	HSR_INTERNAL_PORT,
} hsr_port_type;

/*
 * \ingroup init
 * HSR domain configuration
 */
struct hsr_config {
	bool hsr_enabled;
	uint8_t port_max;
	struct {
		unsigned int logical_port;
		hsr_port_type type;
	} hsr_port[CFG_MAX_LOGICAL_PORTS];
};

/**
 * \endcond
 */

/** Initialize the GenAVB/TSN library.
 * \ingroup init
 * \return		::GENAVB_SUCCESS or negative error code. In case of success genavb argument is updated with library handle
 * \param genavb	pointer to GenAVB/TSN library handle pointer
 * \param flags		bitmap of configuration options. For future expansion, for now always pass 0.
 */
int genavb_init(struct genavb_handle **genavb, unsigned int flags);


/** Exit the GenAVB/TSN library.
 * \ingroup init
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param genavb	pointer to GenAVB/TSN library handle structure
 */
int genavb_exit(struct genavb_handle *genavb);

/* OS specific headers */
#include "os/init.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_INIT_API_H_ */
