/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AEM common definitions
*/

#ifndef _AEM_H_
#define _AEM_H_

#include "common/types.h"
#include "common/adp.h"
#include "common/srp.h"
#include "common/timer.h"
#include "common/random.h"
#include "genavb/aem_helpers.h"
#include "genavb/init.h"
#include "genavb/control_srp.h"
#include "genavb/ptp.h"
#include "adp_milan.h"
#include "adp_ieee.h"
#include "acmp.h"

typedef enum {
	RELEASED,
	ACQUIRED
} entity_acquire_state;

typedef enum {
	UNLOCKED,
	LOCKED
} entity_lock_state;

/* MILAN Discovery, connection and control specification for talkers and listeners rev1.1a (Section 6.4)
 * The List of the controllers registred for unsolicited notifications is stored in the aecp_ctx
 */
struct entity_dynamic_desc {
	entity_lock_state lock_status;				/**< Whether a controller has temporarily locked this entity or not. */
	u64 locking_controller_id;				/**< ID of the controller currently owning (having locked) this entity, if any. (in network order) */

	entity_acquire_state acquire_status;			/**< Whether a controller has acquired this entity or not. */
	u64 acquiring_controller_id;				/**< ID of the controller currently owning (having acquired) this entity, if any. (in network order) */

	struct timer lock_timer;				/**< Timer used for locking timeout */
};

/* MILAN Discovery, connection and control specification for talkers and listeners rev1.1 a (Section 6.6) */
struct avb_interface_dynamic_desc {
	u64 gptp_grandmaster_id;
	u32 propagation_delay;

	struct msrp_pdu_fv_domain msrp_domain[CFG_SR_CLASS_MAX];

	/* AVB interface diagnostic counters as per Milan v1.2, section 5.3.6.3 */
	struct {
		u32 link_up;
		u32 link_down;
		u32 gptp_gm_changed;
	} diagnostic_counters;

	bool operational_state;		/**< Link state of the interface (up/down) */
	bool as_capable;

	u16 max_ptlv_entries;
	u16 num_ptlv_entries;

	struct entity *entity; /**< Pointer to the parent entity struct */
	u16 interface_index;   /**< Interface index of this AVB interface */

	struct timer async_get_as_path_unsolicited_notification_timer; /**< Timer used to regulate AECP GET_AS_PATH async unsolicited notification upon path_sequence changes. */
	struct timer async_get_counters_unsolicited_notification_timer; /**< Timer used to regulate AECP GET_COUNTERS async unsolicited notification upon descriptor changes. */

	u8 async_get_as_path_unsolicited_notification_pending:1; /**< Flag used to state if a GET_AS_PATH unsolicited notification is registered to be sent (on next timer expiration). */
	u8 async_get_as_path_unsolicited_notification_timer_running:1; /**< Flag used to state if the timer is started. */
	u8 async_get_counters_unsolicited_notification_pending:1; /**< Flag used to state if a GET_COUNTERS unsolicited notification is registered to be sent (on next timer expiration). */
	u8 async_get_counters_unsolicited_notification_timer_running:1; /**< Flag used to state if the timer is started. */
	u8 reserved:4;

	union {
		struct {
			adp_milan_advertise_state_t state;	/**< ADP advertise state */
			struct timer adp_delay_timer;		/**< TMR_DELAY: random between 0 and 4 sec */
			struct timer adp_advertise_timer;	/**< TMR_ADVERTISE: 5 sec */
		} milan;

		struct {
			adp_ieee_advertise_interface_state_t advertise_state;	/**< ADP interface advertise state */
		} ieee;
	} u;

	struct ptp_clock_identity path_sequence[]; /**< Variable size array ruled by avdecc_entity_cfg->max_ptlv_entries */
};

struct listener_pair;

struct srp_talker_params {
	u64 stream_id;		/**< (in network order) */
	u16 vlan_id;		/**< (in network order) */
	u8 stream_dest_mac[6];	/**< (in network order) */
};

/* Common structure between AVDECC IEEE 1722.1 TalkerStreamInfos and MILAN Discovery, connection
 * and control specification for talkers and listeners rev1.1a (Section 6.7)
 */
struct stream_output_dynamic_desc {

	u64 stream_id;		/**< (in network order) */
	u16 stream_vlan_id;	/**< (in network order) */
	u8 stream_dest_mac[6];	/**< (in network order) */

	sr_class_t stream_class;

	u32 presentation_time_offset;

	/* Stream output diagnostic counters as per Milan v1.2, section 5.3.7.7 */
	struct {
		u32 stream_start;
		u32 stream_stop;
		u32 media_reset;
		u32 timestamp_uncertain;
		u32 frame_tx;
	} diagnostic_counters;

	struct entity *entity; /**< Pointer to the parent entity struct */
	u16 unique_id;         /**< Unique ID of the talker source */

	struct timer async_get_counters_unsolicited_notification_timer; /**< Timer used to regulate AECP GET_COUNTERS async unsolicited notification upon descriptor changes. */

	u8 async_get_counters_unsolicited_notification_pending:1; /**< Flag used to state if a GET_COUNTERS unsolicited notification is registered to be sent (on next timer expiration). */
	u8 async_get_counters_unsolicited_notification_timer_running:1; /**< Flag used to state if the timer is started. */
	u8 reserved:6;

	union {
		struct {
			genavb_talker_stream_declaration_type_t srp_talker_declaration_type; 	/**< SRP Talker declared attribute state (6.7.2)*/
			genavb_talker_stream_status_t srp_listener_status;			/**< SRP Listener registered attribute state (6.7.2)*/

			struct timer probe_tx_reception_timer; /**< 15 sec timer started on PROBE_TX reception */
			struct timer srp_talker_withdraw_timer; /**< Timer used to wait for two Leaveall period (on MAAP conflict or domain priority code change) on attribute withdraw before redeclaring it. */

			u8 talker_declared:1;
			u8 avtp_connected:1;
			u8 maap_started:1;
			u8 probe_tx_valid:1; /**< True if we received a PROBE_TX_COMMAND in the last 15 sec. */
			u8 srp_talker_withdraw_in_progress:1; /**< True if we are still waiting two LeaveALL periods after withdrawing the talker attribute */
			u8 reserved:3;

			struct srp_talker_params srp_talker_withdraw_params; /**< Contains SRP talker params in withdrawal phase. Valid only if srp_talker_withdraw_in_progress is True */

			struct timer async_unsolicited_notification_timer; /**< Timer used to regulate AECP PDUs sent as async unsolicited notification upon descriptor changes. */

			struct msrp_failure_information failure;		/**< The MSRP failure information */
		} milan;

		struct {
			u16 connection_count;
			struct listener_pair *listeners;
		} ieee;
	} u;
};

/* Common structure between AVDECC IEEE 1722.1 ListenerStreamInfos and MILAN Discovery, connection
 * and control specification for talkers and listeners rev1.1a (Section 6.8)
 */
struct stream_input_dynamic_desc {

	u64 talker_entity_id;		/**< (in network order) */
	u64 controller_entity_id;	/**< (in network order) */
	u64 stream_id;			/**< (in network order) */
	u16 talker_unique_id;		/**< (in network order) */
	u16 flags;			/**< (in network order) */
	u16 stream_vlan_id;		/**< (in network order) */
	u8 stream_dest_mac[6];		/**< (in network order) */

	/* Stream input diagnostic counters as per Milan v1.2, section 5.3.8.10 */
	struct {
		u32 media_locked;
		u32 media_unlocked;
		u32 stream_interrupted;
		u32 seq_num_mismatch;
		u32 media_reset;
		u32 timestamp_uncertain;
		u32 unsupported_format;
		u32 late_timestamp;
		u32 early_timestamp;
		u32 frame_rx;
	} diagnostic_counters;

	struct entity *entity; /**< Pointer to the parent entity struct */
	u16 unique_id;         /**< Unique ID of the listener sink */

	struct timer async_get_counters_unsolicited_notification_timer; /**< Timer used to regulate AECP GET_COUNTERS async unsolicited notification upon descriptor changes. */

	u8 async_get_counters_unsolicited_notification_pending:1; /**< Flag used to state if a GET_COUNTERS unsolicited notification is registered to be sent (on next timer expiration). */
	u8 async_get_counters_unsolicited_notification_timer_running:1; /**< Flag used to state if the timer is started. */
	u8 reserved:6;

	union {
		struct {
			acmp_milan_listener_sink_sm_state_t state;

			struct timer acmp_retry_timer; 			/**< TMR_RETRY: 4 seconds */
			struct timer acmp_delay_timer; 			/**< TMR_DELAY: random between 0 and 1 sec */
			struct timer acmp_talker_registration_timer;	/**< TMR_NO_TK: 10 seconds */

			adp_milan_listener_sink_talker_state_t talker_state;		/**< Talker's discovered state (6.8.4) */
			struct timer adp_discovery_timer;		/**< TMR_NO_ADP: depends on the talker's PDU valid_time */

			genavb_listener_stream_status_t srp_stream_status; 	/**< SRP stream state (6.8.8)*/
			acmp_milan_listener_sink_srp_state_t srp_state; 	/**< SRP sink state: used to track transition between Registering <-> Not registering */

			u32 msrp_accumulated_latency;				/**< The accumulated_latency from the talker advertise. */
			struct msrp_failure_information failure;		/**< The MSRP failure information from the talker failed. */

			struct timer async_unsolicited_notification_timer; /**< Timer used to regulate AECP PDUs sent as async unsolicited notification upon descriptor changes. */

			u8 probing_status;	/**< Probing status (6.8.6) ::acmp_milan_probing_status_t */
			u8 acmp_status;		/**< ACMP status (6.8.6) (in network order) */

			u16 probe_tx_seq_id;	/**< Sequence ID of the sent PROBE_TX command */

			u16 interface_index;		/**< Talker's last received ADP ENTITY_AVAILABLE interface index (in network order)*/
			u32 available_index;		/**< Talker's last received ADP ENTITY_AVAILABLE available index (in network order) */
		} milan;

		struct {
			u8 connected;
			unsigned int flags_priv;
		} ieee;
	} u;
};

/* CLOCK_DOMAIN dynamic state, as per Milan v1.2, section 5.3.11 */
struct clock_domain_dynamic_desc {
	/* Clock domain diagnostic counters as per Milan v1.2, section 5.3.11.2 */
	struct {
		u32 locked;
		u32 unlocked;
	} diagnostic_counters;

	struct entity *entity; /**< Pointer to the parent entity struct */
	u16 unique_id;         /**< Unique ID of the clock_domain */

	struct timer async_get_counters_unsolicited_notification_timer; /**< Timer used to regulate AECP GET_COUNTERS async unsolicited notification upon descriptor changes. */

	u8 async_get_counters_unsolicited_notification_pending:1; /**< Flag used to state if a GET_COUNTERS unsolicited notification is registered to be sent (on next timer expiration). */
	u8 async_get_counters_unsolicited_notification_timer_running:1; /**< Flag used to state if the timer is started. */
	u8 reserved:6;
};

struct aem_desc_hdr *aem_entity_static_init(void);
void aem_init(struct aem_desc_hdr *aem_desc, struct avdecc_entity_config *cfg, int entity_num);
void *aem_dynamic_descs_init(struct aem_desc_hdr *aem_descs, struct avdecc_entity_config *cfg);

#endif /* _AEM_H_ */
