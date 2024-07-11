/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mrp.h
  @brief		MRP common definitions
  @details	All prototypes and definitions necessary for SRP based protocols (MSRP, MVRP, MMRP)
  		are provided within this header file.
*/


#ifndef _MRP_H_
#define _MRP_H_

#include "common/net.h"
#include "common/timer.h"
#include "common/srp.h"
#include "common/log.h"


/**
 * MRP application identifier
 */
#define APP_MSRP 0
#define APP_MVRP 1
#define APP_MMRP 2

/**
 * MRP default priorities per class
 */
#define MRP_DEFAULT_CLASS_A_PRIO	3
#define MRP_DEFAULT_CLASS_B_PRIO	2


/**
 * Registrar states for an attribute
 */
#define RS_NOT	0	/**< Entry not used */
#define RS_IN	1	/**< Atribute registered */
#define RS_LV	2	/**< Previously registered, now being timed out */
#define RS_MT	3	/**< Not registered */

/**
 * Applicant states for an attribute
 */
#define PS_NOT		0	/**< Entry not used */
#define PS_EMPTY	1	/**< Atribute not declared */
#define PS_JOIN		2	/**< Atribute declared */


/** MRP participants types
 * A Full Participant implements the complete Applicant state machine (Table 10-3) and the Registrar state
 * machine (Table 10-4) for each Attribute declared, registered, or tracked, together with a single instance of
 * the LeaveAll state machine (Table 10-5) and the PeriodicTransmission state machine (Table 10-6).
 *
 * An Applicant-Only Participant implements the Applicant state machine, with the omission of certain states
 * and actions as specified by Table 10-3, for each Attribute declared, registered, or tracked, together with a
 * single instance of the PeriodicTransmission state machine (Table 10-6).
 */
#define MRP_PARTICIPANT_TYPE_FULL		(1 << 0)
#define MRP_PARTICIPANT_TYPE_APPLICANT_ONLY 	(0 << 0)
#define MRP_PARTICIPANT_TYPE_POINT_TO_POINT	(1 << 1)

#define MRP_PARTICIPANT_TYPE_FULL_POINT_TO_POINT 	(MRP_PARTICIPANT_TYPE_FULL | MRP_PARTICIPANT_TYPE_POINT_TO_POINT)
#define MRP_PARTICIPANT_TYPE_APPLICANT_POINT_TO_POINT	(MRP_PARTICIPANT_TYPE_APPLICANT_ONLY | MRP_PARTICIPANT_TYPE_POINT_TO_POINT)


/**
 *  Timers states
 */
typedef enum {
	MRP_TIMER_PASSIVE = 0,
	MRP_TIMER_ACTIVE
} mrp_protocol_timer_state_t;


/**
 *  Attribute event definitions
 */
typedef enum {
	MRP_ATTR_EVT_NEW = 0,
	MRP_ATTR_EVT_JOININ,
	MRP_ATTR_EVT_IN,
	MRP_ATTR_EVT_JOINMT,
	MRP_ATTR_EVT_MT,
	MRP_ATTR_EVT_LV,
	MRP_ATTR_EVT_LVA,
	MRP_ATTR_EVT_MAX,
	MRP_ATTR_EVT_ERR = 0xFF
} mrp_protocol_attribute_event_t;

/**
 *  LeaveAll event definitions
 */
typedef enum {
	MRP_NULL_LVA_EVENT = 0,
	MRP_LVA_EVENT
} mrp_protocol_leave_all_event_t;


/**
 * Paticipant and Registrar states (802.1Q - 10.7.1)
 */
typedef enum {
	/**
	*paticipant states
	*/
	MRP_STATE_VO = 0,	/**< Very Anxious Observer */
	MRP_STATE_VP,		/**< Very Anxious Passive */
	MRP_STATE_VN,		/**< Very Anxious New */
	MRP_STATE_AN,		/**< Anxious New */
	MRP_STATE_AA,		/**< Anxious Active */
	MRP_STATE_QA,		/**< Quiet Active */
	MRP_STATE_LA,		/**< Leaving Active */
	MRP_STATE_AO,		/**< Anxious Observer State */
	MRP_STATE_QO,		/**< Quiet Observer State */
	MRP_STATE_AP,		/**< Anxious Passive State */
	MRP_STATE_QP,		/**< Quiet Passive State */
	MRP_STATE_LO,		/**< Leaving Observer State */

	/**
	*Registrar states
	*/
	MRP_STATE_IN,		/**< In */
	MRP_STATE_LV,		/**< Leaving */
	MRP_STATE_MT		/**< Empty */
} mrp_protocol_state_t;


/**
 * MRP protocol events (802.1Q - 10.7.1)
 */
typedef enum {
	MRP_EVENT_BEGIN = 0,	/**< Initialize state machine */

	/* transmitted event to network */
	MRP_EVENT_NEW,		/**< A new declaration */
	MRP_EVENT_JOIN,		/**< Declaration without signaling new registration */
	MRP_EVENT_LV,		/**< Withdraw a declaration */
	MRP_EVENT_TX,		/**< Tranmission opportunity without a LeaveAll */
	MRP_EVENT_TXLA,		/**< Tranmission opportunity with a LeaveAll */
	MRP_EVENT_TXLAF,	/**< Tranmission opportunity with a LeaveAll,and with no room (Full) */

	/* received event from network  */
	MRP_EVENT_RNEW,		/**< receive New message */
	MRP_EVENT_RJOININ,	/**< receive JoinIn message */
	MRP_EVENT_RIN,		/**< receive In message */
	MRP_EVENT_RJOINMT,	/**< receive JoinEmpty message  */
	MRP_EVENT_RMT,		/**< receive Empty message */
	MRP_EVENT_RLV,		/**< receive Leave message */
	MRP_EVENT_RLA,		/**< receive a LeaveAll message */

	MRP_EVENT_FLUSH,	/**< Port role changes from Root Port or Alternate Port to Designated Port */
	MRP_EVENT_REDECLARE,	/**< Port role changes from Designated to Root Port or Alternate Port */

	/* events from timers */
	MRP_EVENT_PERIODIC,		/**< A periodic transmission event occurs */
	MRP_EVENT_LEAVETIMER,		/**< leave timer expire */
	MRP_EVENT_LEAVEALLTIMER,	/**< leaveall timer has expired */
	MRP_EVENT_PERIODICTIMER,	/**< periodic timer has expired */

	MRP_EVENT_PERIODIC_ENABLED,	/**< Indicates to the Periodic transmission state machine that is has been enabled by management action */
	MRP_EVENT_PERIODIC_DISABLED,	/**< Indicates to the Periodic transmission state machine that is has been disabled by management action */

	MRP_EVENT_RLV_IMMEDIATE
} mrp_protocol_event_t;


/**
 * MRP protocol actions (802.1Q - 10.7.1)
 */
typedef enum {
	MRP_ACTION_NONE = 0,
	MRP_ACTION_NEW,		/**< send a New indication to MAP and the MRP application */
	MRP_ACTION_JOIN,	/**< send a Join indication to MAP and the MRP application */
	MRP_ACTION_LV,		/**< send a Lv indication to MAP and the MRP application */
	MRP_ACTION_SN,		/**< send a New message */
	MRP_ACTION_SJ,		/**< send a JoinIn or JoinMt message */
	MRP_ACTION_SJ_,		/**< send a JoinIn or JoinMt message */
	MRP_ACTION_SL,		/**< send a Lv message */
	MRP_ACTION_S,		/**< send a In or Empty message */
	MRP_ACTION_S_,		/**< send a In or Empty message */
	MRP_ACTION_SLA,		/**< send a Leave All message */
	MRP_ACTION_PERIODIC,	/**< Periodic transmission event */
	MRP_ACTION_LEAVETIMER,	/**< Leave All period timer */
	MRP_ACTION_LEAVEALLTIMER,	/**< Leave All timer */
	MRP_ACTION_PERIODICTIMER	/**< Periodic transmission timer */
} mrp_protocol_action_t;


/* applicant and registrar definition used by all MRP applications */

/**
 *  MRP applicant definition
 */
struct mrp_applicant {
	mrp_protocol_state_t state;
	mrp_protocol_action_t action;
};

/**
 * MRP leave definition
 */
struct mrp_leave {
	struct timer timer;
};

/**
 * MRP registrar definition
 */
struct mrp_registrar {
	mrp_protocol_state_t state;
	mrp_protocol_action_t action;
	struct mrp_leave leave;
};

/**
 * MRP leaveall definition
 */
struct mrp_leaveall {
	mrp_protocol_timer_state_t state;
	mrp_protocol_action_t action;
	struct timer timer;
};

/**
 *  MRP periodic definition
 */
struct mrp_periodic {
	mrp_protocol_timer_state_t state;
	mrp_protocol_action_t action;
	struct timer timer;
};


/**
 * MRP join definition
 */
struct mrp_join {
	struct timer timer;
};


/**
 * MRP object management flags
 */
#define MRP_FLAG_DECLARED	(1 << 0)	/**< attribute declared and sent from local application */
#define MRP_FLAG_REGISTERED	(1 << 1)	/**< attribute received and registered from network */


/* MRP generic structure to track an mrp pdu */
struct mrp_pdu {
	void *start;
	void *msg_start;
	void *end;
};

/* MRP generic structure to track an mrp message */
struct mrp_msg {
	u8 *start;
	u8 *vector_start;
	u8 *end;
};

/* MRP generic structure to track an mrp vector */
struct mrp_vector {
	u8 *start;
	u8 *event_start;
	u8 *end;
	unsigned int number_of_values;
	unsigned int leave_all_event;
};

/**
 * MRP generic attribute definition
 */
struct mrp_attribute {
	struct list_head list;
	struct list_head list_app;
	struct mrp_application *app;	/**< pointer to the associated MRP application (MSRP, MVRP, MMRP) */
	struct mrp_applicant applicant;	/**< applicant state machine context for this attribute  */
	struct mrp_registrar registrar; /**< registrar state machine context for this attribute  */
	u8 type;			/**< attribute type (application dependent) */
	u8 val[0];			/**< attribute value */
};

struct mrp_attribute_mrpdu {
	struct net_tx_desc *desc;
	struct mrp_msg msg;
	struct mrp_vector vector;
};

#define MRP_MAX_ATTR_TYPE	5

/**
 *  MRP application definition
 */
struct mrp_application {
	struct srp_ctx *srp;		/**< pointer to the main SRP context */
	unsigned int type;		/**< application type (MSRP, MVRP, MMRP) */
	unsigned int port_id;		/**< port ID useful for log printing */
	unsigned int participant_type;	/**< specify if the application is acting as full participant of as applicant only */
	struct mrp_leaveall leaveall;	/**< leaveall state machine context common to all attribute of the application */

	struct list_head attributes[MRP_MAX_ATTR_TYPE];	/**< chained list of attributes associated to this application */

	/* FIXME this is wrong, periodic timer instance is per port, not per participant */
	struct mrp_periodic periodic;
	struct mrp_join join;

	void (*mad_join_indication)(struct mrp_application *app, struct mrp_attribute *attr, bool new);	/**< application specific send join indication function */
	void (*mad_leave_indication)(struct mrp_application *app, struct mrp_attribute *attr);			/**< application specific send leave function */

	unsigned int (*attribute_length)(unsigned int attribute_type);
	unsigned int (*attribute_value_length)(unsigned int attribute_type);
	void (*vector_add_event)(struct mrp_attribute *attr, struct mrp_vector *vector, mrp_protocol_attribute_event_t event);
	void (*net_tx)(struct mrp_application *app, struct net_tx_desc *desc);
	struct net_tx_desc * (*net_tx_alloc)(struct mrp_application *app, unsigned int size);

	const void *dst_mac;
	unsigned int ethertype;
	unsigned int min_attr_type;
	unsigned int max_attr_type;

	unsigned int num_rx_pkts;
	unsigned int proto_version;
	unsigned int has_attribute_list_length;

	bool enabled;

	struct mrp_attribute_mrpdu mrpdu[MRP_MAX_ATTR_TYPE];	/* pointer to current MRPDU, for a specific attribute type */

	int (*attribute_check)(struct mrp_pdu_header *mrp_header);
	int (*event_length)(struct mrp_pdu_header *mrp_header, unsigned int number_of_values);
	int (*vector_handler)(struct mrp_application *app, struct mrp_pdu_header *mrp_header, void *vector_data, unsigned int number_of_values);
	const char *(*attribute_type_to_string)(unsigned int type);
};

struct mrp_attribute *mrp_mad_join_request(struct mrp_application *, unsigned int type, u8 *val, bool new);
void mrp_mad_leave_request(struct mrp_application *, unsigned int type, u8 *val);

int mrp_process_packet(struct mrp_application *app, struct net_rx_desc *desc);

void mrp_enable(struct mrp_application *app);
void mrp_disable(struct mrp_application *app);
int mrp_init(struct mrp_application *, unsigned int, unsigned int);
int mrp_exit(struct mrp_application *);

int mrp_process_attribute(struct mrp_application *app, unsigned int type, u8 *val, mrp_protocol_attribute_event_t);
int mrp_process_attribute_leave_immediate(struct mrp_attribute *attr);

const char *mrp_attribute_event2string (mrp_protocol_attribute_event_t attribute_event);
const char *mrp_listener_declaration2string(msrp_listener_declaration_type_t declaration_type);
const char *mrp_talker_declaration2string(msrp_talker_declaration_type_t declaration_type);

static inline void mrp_get_three_packed_event(u8 threepackedevent, unsigned int *event)
{
	event[0] = threepackedevent / (6 * 6);
	threepackedevent -= event[0] * (6 * 6);
	event[1] = threepackedevent / 6;
	threepackedevent -= event[1] * 6;
	event[2] = threepackedevent;
}

static inline void mrp_get_four_packed_event(u8 fourpackedevent, unsigned int *event)
{
	event[0] = fourpackedevent / 64;
	fourpackedevent -= event[0] * 64;
	event[1] = fourpackedevent / 16;
	fourpackedevent -= event[1] * 16;
	event[2] = fourpackedevent / 4;
	fourpackedevent -= event[2] * 4;
	event[3] = fourpackedevent;
}

void mrp_vector_add_event(struct mrp_vector *vector, unsigned int event);
void mrp_vector_add_event_four(struct mrp_vector *vector, unsigned int event, unsigned int value);

#endif /* _MRP_H_ */
