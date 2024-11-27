/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mrp.c
  @brief	MRP (MSRP, MSRP) common code
  @details	implements common function for MRP attributes management
*/


#include "os/stdlib.h"
#include "os/string.h"

#include "common/log.h"
#include "common/random.h"

#include "srp.h"
#include "mrp.h"

static int __mrp_process_attribute(struct mrp_attribute *attr, mrp_protocol_attribute_event_t event);
static struct mrp_attribute *mrp_find_attribute(struct mrp_application *app, unsigned int type, u8 *val);
static struct mrp_attribute *mrp_get_attribute(struct mrp_application *app, unsigned int type, u8 *val);
static struct mrp_attribute *mrp_alloc_attribute(struct mrp_application *app, unsigned int type, u8 *val);
static void mrp_free_attribute(struct mrp_attribute *attr);

static const mrp_protocol_event_t mrp_attribute2rxevent [MRP_ATTR_EVT_MAX] = {
	[MRP_ATTR_EVT_NEW] 	= MRP_EVENT_RNEW,
	[MRP_ATTR_EVT_JOININ] 	= MRP_EVENT_RJOININ,
	[MRP_ATTR_EVT_IN] 	= MRP_EVENT_RIN,
	[MRP_ATTR_EVT_JOINMT] 	= MRP_EVENT_RJOINMT,
	[MRP_ATTR_EVT_MT] 	= MRP_EVENT_RMT,
	[MRP_ATTR_EVT_LV] 	= MRP_EVENT_RLV,
	[MRP_ATTR_EVT_LVA] 	= MRP_EVENT_RLA
};


static const char * mrp_state2string(mrp_protocol_state_t state)
{
	switch (state) {
		/* applicant */
		case2str(MRP_STATE_VO);
		case2str(MRP_STATE_VP);
		case2str(MRP_STATE_VN);
		case2str(MRP_STATE_AN);
		case2str(MRP_STATE_AA);
		case2str(MRP_STATE_QA);
		case2str(MRP_STATE_LA);
		case2str(MRP_STATE_AO);
		case2str(MRP_STATE_QO);
		case2str(MRP_STATE_AP);
		case2str(MRP_STATE_QP);
		case2str(MRP_STATE_LO);
		/*registrar */
		case2str(MRP_STATE_IN);
		case2str(MRP_STATE_LV);
		case2str(MRP_STATE_MT);
		default:
			return (char *) "Unknown MRP state";
	}
}

static const char *mrp_action2string(mrp_protocol_action_t action)
{
	switch (action) {
		case2str(MRP_ACTION_NONE);
		case2str(MRP_ACTION_NEW);
		case2str(MRP_ACTION_JOIN);
		case2str(MRP_ACTION_LV);
		case2str(MRP_ACTION_SN);
		case2str(MRP_ACTION_SJ);
		case2str(MRP_ACTION_SJ_);
		case2str(MRP_ACTION_SL);
		case2str(MRP_ACTION_S);
		case2str(MRP_ACTION_S_);
		case2str(MRP_ACTION_SLA);
		case2str(MRP_ACTION_PERIODIC);
		case2str(MRP_ACTION_LEAVETIMER);
		case2str(MRP_ACTION_LEAVEALLTIMER);
		case2str(MRP_ACTION_PERIODICTIMER);
		default:
			return (char *) "Unknown MRP action";
	}
}

const char *mrp_attribute_event2string(mrp_protocol_attribute_event_t attribute)
{
	switch (attribute) {
		case2str(MRP_ATTR_EVT_NEW);
		case2str(MRP_ATTR_EVT_JOININ);
		case2str(MRP_ATTR_EVT_IN);
		case2str(MRP_ATTR_EVT_JOINMT);
		case2str(MRP_ATTR_EVT_MT);
		case2str(MRP_ATTR_EVT_LV);
		case2str(MRP_ATTR_EVT_LVA);
		case2str(MRP_ATTR_EVT_ERR);
		default:
			return (char *) "Unknown MRP attribute event";
	}
}

const char *mrp_talker_declaration2string(msrp_talker_declaration_type_t declaration_type)
{
	switch (declaration_type) {
		case2str(MSRP_TALKER_DECLARATION_TYPE_ADVERTISE);
		case2str(MSRP_TALKER_DECLARATION_TYPE_FAILED);
		default:
			return (char *) "Unknown talker declaration type";
	}
}


const char *mrp_listener_declaration2string(msrp_listener_declaration_type_t declaration_type)
{
	switch (declaration_type) {
		case2str(MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED);
		case2str(MSRP_LISTENER_DECLARATION_TYPE_READY);
		case2str(MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED);
		default:
			return (char *) "Unknown listener declaration type";
	}
}

static const char *mrp_timer_state2string(mrp_protocol_timer_state_t state)
{
	switch (state) {
		case2str(MRP_TIMER_PASSIVE);
		case2str(MRP_TIMER_ACTIVE);
		default:
			return (char *) "Unknown MRP timer state";
	}
}

static const char *mrp_event2string(mrp_protocol_event_t event)
{
	switch (event) {
		case2str(MRP_EVENT_BEGIN);
		case2str(MRP_EVENT_NEW);
		case2str(MRP_EVENT_JOIN);
		case2str(MRP_EVENT_LV);
		case2str(MRP_EVENT_TX);
		case2str(MRP_EVENT_TXLA);
		case2str(MRP_EVENT_TXLAF);
		case2str(MRP_EVENT_RNEW);
		case2str(MRP_EVENT_RJOININ);
		case2str(MRP_EVENT_RIN);
		case2str(MRP_EVENT_RJOINMT);
		case2str(MRP_EVENT_RMT);
		case2str(MRP_EVENT_RLV);
		case2str(MRP_EVENT_RLA);
		case2str(MRP_EVENT_FLUSH);
		case2str(MRP_EVENT_REDECLARE);
		case2str(MRP_EVENT_PERIODIC);
		case2str(MRP_EVENT_LEAVETIMER);
		case2str(MRP_EVENT_LEAVEALLTIMER);
		case2str(MRP_EVENT_PERIODICTIMER);
		case2str(MRP_EVENT_PERIODIC_ENABLED);
		case2str(MRP_EVENT_PERIODIC_DISABLED);
		default:
			return (char *) "Unknown MRP event";
	}
}


static int mrp_action2event(struct mrp_application *app, struct mrp_attribute *attr)
{
	struct mrp_applicant *applicant = &attr->applicant;
	struct mrp_registrar *registrar = &attr->registrar;
	int event;

	switch (applicant->action) {
	case MRP_ACTION_SN:
		event = MRP_ATTR_EVT_NEW;
		break;

	case MRP_ACTION_SJ:
		if (app->participant_type & MRP_PARTICIPANT_TYPE_FULL) {
			if (registrar->state == MRP_STATE_IN)
				event = MRP_ATTR_EVT_JOININ;
			else
				event = MRP_ATTR_EVT_JOINMT;
		} else
			event = MRP_ATTR_EVT_JOINMT;
		break;

	case MRP_ACTION_SL:
		event = MRP_ATTR_EVT_LV;
		break;

	case MRP_ACTION_S:
		if (app->participant_type & MRP_PARTICIPANT_TYPE_FULL) {
			if (registrar->state == MRP_STATE_IN)
				event = MRP_ATTR_EVT_IN;
			else
				event = MRP_ATTR_EVT_MT;
		} else
			event = MRP_ATTR_EVT_MT;

		break;

	case MRP_ACTION_S_:
	case MRP_ACTION_SJ_:
		/* FIXME for now do nothing */
	default:
		event = -1;
		break;
	}

	return event;
}

static struct mrp_pdu_header *mrp_get_first_message(struct mrp_application *app, u8 *data, u8 *end)
{
	data += 1;

	if (app->has_attribute_list_length) {
		if ((data + sizeof(struct mrp_pdu_header)) > end)
			return NULL;
	} else {
		if ((data + sizeof (struct mvrp_pdu_header)) > end)
			return NULL;
	}

	return (struct mrp_pdu_header *)data;
}

static struct mrp_pdu_header *mrp_get_next_message(u16 *vector_end_mark, u8 *end)
{
	struct mrp_pdu_header *mrp_header = (struct mrp_pdu_header *)(vector_end_mark + 1);

	/* Check that the end marker is valid */
	if (((u8 *) mrp_header + 2) > end) {
		os_log(LOG_ERR, "Trunkated MRP PDU, no end marker\n");
		return NULL;
	}

	/* End marker */
	if (*(u16 *)((u8 *)mrp_header) == 0x0000)
		return NULL;

	/* Check that next MRP header is valid */
	if (((u8 *)(mrp_header + 1)) > end) {
		os_log(LOG_ERR, "Trunkated MRP PDU, next MRP header invalid\n");
		return NULL;
	}

	return mrp_header;
}

static u16 *mrp_get_first_vector(struct mrp_application *app, struct mrp_pdu_header *mrp_header)
{
	if (app->has_attribute_list_length)
		return (u16 *)(mrp_header + 1);
	else
		return (u16 *)((struct mvrp_pdu_header *)mrp_header + 1);
}

static void mrp_get_vector_header(u16 vector_header, unsigned int *leave_all_event, unsigned int *number_of_values)
{
	vector_header = ntohs(vector_header);

	*leave_all_event = vector_header / 8192;
	*number_of_values = vector_header % 8192;
}

static int mrp_msg_start(struct mrp_msg *msg, struct mrp_application *app, u8 *buf, u8 *buf_end, unsigned int attribute_type, unsigned int attribute_length)
{
	msg->start = buf;
	msg->vector_start = msg->start;
	msg->end = msg->vector_start;

	/* Remove message's end mark size from the message buffer limit to already take it into account */
	msg->end_max = buf_end - sizeof(u16);

	if (app->has_attribute_list_length) {
		struct mrp_pdu_header *mrp_hdr = (struct mrp_pdu_header *)msg->start;
		/* Make sure the MRP message has enough space for its header and end mark */
		u8 *end_mrp = (u8 *)msg->start + sizeof(struct mrp_pdu_header);

		/* Size of msg exceed desc's limit */
		if (end_mrp > msg->end_max)
			goto err;

		mrp_hdr->attribute_type = attribute_type;
		mrp_hdr->attribute_length = attribute_length;
		mrp_hdr->attribute_list_length = 0;

		msg->vector_start += sizeof(struct mrp_pdu_header);

	} else {
		struct mvrp_pdu_header *mrp_hdr = (struct mvrp_pdu_header *)msg->start;
		/* Make sure the MVRP message has enough space for its header and end mark */
		u8 *end_mvrp = (u8 *)msg->start + sizeof(struct mvrp_pdu_header);

		/* Size of msg exceed desc's limit */
		if (end_mvrp > msg->end_max)
			goto err;

		mrp_hdr->attribute_type = attribute_type;
		mrp_hdr->attribute_length = attribute_length;

		msg->vector_start += sizeof(struct mvrp_pdu_header);
	}

	msg->end = msg->vector_start;

	return 0;

err:
	return -1;
}

static unsigned int mrp_msg_end(struct mrp_msg *msg, struct mrp_application *app)
{
	if (app->has_attribute_list_length) {
		struct mrp_pdu_header *mrp_hdr = (struct mrp_pdu_header *)msg->start;
		unsigned int len = msg->end - msg->vector_start;

		mrp_hdr->attribute_list_length = htons(len);
	}

	/* Message endmark */
	*(u16 *)msg->end = 0;
	msg->end += 2;

	return msg->end - msg->start;
}


static int mrp_vector_start(struct mrp_vector *vector, u8 *buf, u8 *buf_end, unsigned int fv_len)
{
	u16 *vector_header;
	u8 *fv_start;
	u8 *end;

	vector->start = buf;
	vector->end = vector->start;

	/* Remove vector's end mark size from the vector buffer limit to already take it into account */
	vector->end_max = buf_end - sizeof(u16);

	/* Make sure the vector has enough space for at least its header, firstValue and end mark*/
	end = (u8 *)vector->start + sizeof(u16) + fv_len;

	/* Size of vector exceeds the msg's size. We can't add this new vector */
	if (end > vector->end_max)
		goto err;

	vector_header = (u16 *)vector->start;
	fv_start = (u8 *)(vector_header + 1);
	os_memset(fv_start, 0, fv_len);

	*vector_header = 0;

	vector->event_start = fv_start + fv_len;
	vector->end = vector->event_start;
	vector->number_of_values = 0;
	vector->leave_all_event = MRP_NULL_LVA_EVENT;

	return 0;

err:
	return -1;
}

static unsigned int mrp_vector_end(struct mrp_vector *vector, unsigned int final)
{
	u16 *end_mark = (u16 *)vector->end;
	unsigned int len;

	if (final) {
		*end_mark = 0;

		vector->end += sizeof(u16);
	}

	len = vector->end - vector->start;

	vector->start = NULL;
	vector->event_start = NULL;
	vector->end = NULL;

	return len;
}

static u16 mrp_vector_header(u8 leave_event_all, u16 number_of_values)
{
	return htons((leave_event_all * 8192) + number_of_values);
}

static void mrp_vector_first_value(struct mrp_vector *vector, void *fv)
{
	u16 *vector_header = (u16 *)vector->start;
	u8 *fv_start = (u8 *)(vector_header + 1);

	os_memcpy(fv_start, fv, vector->event_start - fv_start);
}

static void mrp_vector_leaveall_event(struct mrp_vector *vector, mrp_protocol_leave_all_event_t leave_all_event)
{
	u16 *vector_header = (u16 *)vector->start;

	vector->leave_all_event = leave_all_event;

	*vector_header = mrp_vector_header(vector->leave_all_event, vector->number_of_values);
}

static u8 mrp_three_packed_event(unsigned int event0, unsigned int event1, unsigned int event2)
{
	return ((event0 * 6) + event1) * 6 + event2;
}

static u8 mrp_four_packed_event(unsigned int event0, unsigned int event1, unsigned int event2, unsigned int event3)
{
	return (((event0 * 4) + event1) * 4 + event2) * 4 + event3;
}

int mrp_vector_add_event(struct mrp_vector *vector, unsigned int event)
{
	unsigned int new_number_of_values = vector->number_of_values + 1;
	unsigned int new_event_len = (new_number_of_values + 2) / 3;
	u16 *vector_header = (u16 *)vector->start;
	u8 *end;

	/* Make sure the events with the new one we're adding fit in the vector's size */
	end = (u8 *)vector->event_start + new_event_len;

	/* Size of vector exceeds the msg's size. We can't add any new event to this vector */
	if (end > vector->end_max) {
		goto err;
	}

	vector->number_of_values = new_number_of_values;

	*vector_header = mrp_vector_header(vector->leave_all_event, vector->number_of_values);

	/* FIXME needs to be improved to support more than one value */
	*(vector->event_start) = mrp_three_packed_event(event, 0, 0);

	vector->end = vector->event_start + new_event_len;

	return 0;

err:
	return -1;
}

int mrp_vector_add_event_four(struct mrp_vector *vector, unsigned int event, unsigned int value)
{
	unsigned int new_number_of_values = vector->number_of_values + 1;
	unsigned int new_event_len = (new_number_of_values + 2) / 3 + (new_number_of_values + 3) / 4;
	u16 *vector_header = (u16 *)vector->start;
	u8 *end;

	/* Make sure the events with the new one we're adding fit in the vector's size */
	end = (u8 *)vector->event_start + new_event_len;

	/* Size of vector exceeds the msg's size. We can't add any new event to this vector */
	if (end > vector->end_max) {
		goto err;
	}

	vector->number_of_values = new_number_of_values;

	*vector_header = mrp_vector_header(vector->leave_all_event, vector->number_of_values);

	/* FIXME needs to be improved to support more than one value */
	*(vector->event_start) = mrp_three_packed_event(event, 0, 0);
	*(vector->event_start + 1) = mrp_four_packed_event(value, 0, 0, 0);

	vector->end = vector->event_start + new_event_len;

	return 0;

err:
	return -1;
}


/** Start a 'send join request' timer for all attributes of a given MRP application
 * \return	none
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 */
static void mrp_transmit_request(struct mrp_application *app)
{
	/* FIXME if point to point timer value should be smaller, otherwise randomize value */
	if (!(app->join.timer.flags & TIMER_STATE_STARTED))
		timer_start(&app->join.timer, MRP_JOINTIMER_POINT_TO_POINT_VAL);
}

static void mrp_transmit_end(struct mrp_application *app, unsigned int attribute_type)
{
	struct mrp_attribute_mrpdu *mrpdu = &app->mrpdu[attribute_type];
	struct net_tx_desc *desc;

	desc = mrpdu->desc;
	if (desc) {
		struct mrp_msg *msg = &mrpdu->msg;
		struct mrp_vector *vector = &mrpdu->vector;

		if (vector->start)
			msg->end += mrp_vector_end(vector, 1);

		desc->len += mrp_msg_end(msg, app);

		app->net_tx(app, desc);

		mrpdu->desc = NULL;
	}
}

/* Handles error cases where the MRP message is full and can no longer be packed with more vectors or events.
 * If possible, closes the message and transmit it, while letting the caller start a new message.
 * \return			none
 * \param app			pointer to the MRP application (MSRP, MVRP, MMRP)
 * \param attribute_type	attributes' type to which the message applies (802.1Q-2022, section 10.8.2.2)
 * \param msg			pointer to the MRP message buffer
 * \param vector		pointer to the MRP vector buffer
 */
static void mrp_msg_error(struct mrp_application *app, unsigned int attribute_type, struct mrp_msg *msg, struct mrp_vector *vector)
{
	struct mrp_attribute_mrpdu *mrpdu = &app->mrpdu[attribute_type];

	if (!msg->start) {
		/* The MRP message couldn't be started at all */
		net_tx_free(mrpdu->desc);
		mrpdu->desc = NULL;

	}else {
		if (msg->start == msg->end) {
			/* NOTE: The stack supports one message per MRPDU.
			* Here, the MRP message couldn't be started because its header and EndMark won't fit in the MRPDU (e.g mrp_msg_start() failed)
			* So, free the network descriptor.
			*/
			os_log(LOG_ERR, "mrp_app(%p), desc is too small to start a MRP message\n", app);
			net_tx_free(mrpdu->desc);
			mrpdu->desc = NULL;

		} else {
			if (!vector->start) {
				/* The last MRP vector, if any, is closed */
				mrp_transmit_end(app, attribute_type);

			} else {
				if (vector->start == vector->end) {
					/* The new vector couldn't be started because it couldn't fit in the MRP message (e.g mrp_vector_start() failed) */
					mrp_transmit_end(app, attribute_type);

				} else {
					/* The current vector couldn't fit any more new events so the vector and message should be closed and transmited (e.g app->vector_add_event() failed) */
					if (vector->number_of_values == 0)
						msg->end = vector->start;

					mrp_transmit_end(app, attribute_type);
				}
			}
		}
	}
}

static void mrp_transmit_action(struct mrp_application *app, unsigned int attribute_type, struct mrp_attribute *attr, mrp_protocol_attribute_event_t event, unsigned int leaveall)
{
	struct net_tx_desc *desc;
	void *pdu;
	struct mrp_attribute_mrpdu *mrpdu = &app->mrpdu[attribute_type];
	struct mrp_msg *msg = &mrpdu->msg;
	struct mrp_vector *vector = &mrpdu->vector;
	unsigned int attribute_length = app->attribute_length(attribute_type);
	u8 *msg_end_max;

	os_log(LOG_DEBUG, "(%p, %p) %u %u %p\n", app, attr, attribute_type, attribute_length, mrpdu->desc);

new:
	if (!mrpdu->desc) {
		struct net_tx *tx = &app->srp->port[app->port_id].net_tx;

		desc = app->net_tx_alloc(app, DEFAULT_NET_DATA_SIZE);
		if (!desc) {
			os_log(LOG_ERR,"mrp_app(%p) Cannot alloc tx descriptor\n", app);
			return;
		}

		mrpdu->desc = desc;

		pdu = NET_DATA_START(desc);

		desc->len += net_add_eth_header(pdu, app->dst_mac, app->ethertype);

		msg_end_max = (u8 *)pdu + net_port_mtu_get(tx, app->logical_port_id);

		*((u8 *)pdu + desc->len) = app->proto_version;
		desc->len++;

		if (mrp_msg_start(msg, app, (u8 *)pdu + desc->len, msg_end_max, attribute_type, attribute_length) < 0) {
			mrp_msg_error(app, attribute_type, msg, vector);
			goto exit;
		}
	} else {
		desc = mrpdu->desc;

		pdu = NET_DATA_START(desc);

		if (leaveall)
			os_log(LOG_ERR, "Leaveall event not at start of MSRPDU\n");
	}

	if (!vector->start) {
		if (mrp_vector_start(vector, msg->end, msg->end_max, attribute_length) < 0) {
			/* No space in the current PDU to add another vector: Transmit the current and rebuild a new one. */
			mrp_msg_error(app, attribute_type, msg, vector);
			goto new;
		}
	}

	if (leaveall)
		mrp_vector_leaveall_event(vector, MRP_LVA_EVENT);

	if (attr) {
		if (vector->number_of_values) {
			if ((app->type == APP_MSRP) && (attr->type == MSRP_ATTR_TYPE_DOMAIN)) {
				/* FIXME, don't put multiple domain vectors in the same message, since it causes Violett tests to fail.
				In principle it is expecting domains to be merged in the same vector, which we don't support */
				mrp_transmit_end(app, attribute_type);
				goto new;
			} else {
				/* FIXME, add support for merging multiple events in the same vector */
				msg->end += mrp_vector_end(vector, 0);

				if (mrp_vector_start(vector, msg->end, msg->end_max, attribute_length) < 0) {
					/* No space in the current PDU to add another vector: Transmit the current and rebuild a new one. */
					mrp_msg_error(app, attribute_type, msg, vector);
					goto new;
				}

				mrp_vector_first_value(vector, attr->val);
			}
		} else {
			mrp_vector_first_value(vector, attr->val);
		}

		if (app->vector_add_event(attr, vector, event) < 0) {
			/* No space in the current PDU to add an event: Transmit the current and rebuild a new one. */
			mrp_msg_error(app, attribute_type, msg, vector);
			goto new;
		}
	}

exit:
	return;
}

static void mrp_free_attribute_check(struct mrp_application *app, struct mrp_attribute *attr)
{
	/* 802.1Q-2011, Table 10-3, Note 11 */
	if (((attr->applicant.state == MRP_STATE_VO) ||
	    (attr->applicant.state == MRP_STATE_AO) ||
	    (attr->applicant.state == MRP_STATE_QO)) &&
	    (!(app->participant_type & MRP_PARTICIPANT_TYPE_FULL) || (attr->registrar.state == MRP_STATE_MT)))
		mrp_free_attribute(attr);
}

/** Perform appropriate action depending on applicant state machine output
 * \return	none
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 * \param attr	pointer to the MRP attribute the applicant state machine has been ran for
 */
static void mrp_applicant_action_handler(struct mrp_application *app, struct mrp_attribute *attr)
{
	int event;

	event = mrp_action2event(app, attr);
	if (event < 0)
		return;

	mrp_transmit_action(app, attr->type, attr, event, 0);
}


/** Per Attribute Applicant state machine following 802.1Q Table 10.3 (see notes for Applicant Only exceptions)
 * \return	none
 * \param attr	pointer to the MRP attribute the applicant state machine is updated for
 * \param event	MRP event value. Can originate from timers, received PDU or transmitted declaration
 */
static void mrp_applicant_sm(struct mrp_attribute *attr, mrp_protocol_event_t event)
{
	struct mrp_application *app = attr->app;
	struct mrp_applicant *applicant = &attr->applicant;
	struct mrp_registrar *registrar = &attr->registrar;
	mrp_protocol_state_t state = applicant->state;
	mrp_protocol_action_t action = MRP_ACTION_NONE;

	switch (event) {
	case MRP_EVENT_BEGIN:
		state = MRP_STATE_VO;
		action = MRP_ACTION_NONE;
		break;

	case MRP_EVENT_NEW:
		switch (state) {
		case MRP_STATE_VN:
		case MRP_STATE_AN:
			break;
		default:
			state = MRP_STATE_VN;
			break;
		}
		break;

	case MRP_EVENT_JOIN:
		switch (state) {
		case MRP_STATE_VO:
		case MRP_STATE_LO:
			state = MRP_STATE_VP;
			break;
		case MRP_STATE_LA:
			state = MRP_STATE_AA;
			break;
		case MRP_STATE_AO:
			state = MRP_STATE_AP;
			break;
		case MRP_STATE_QO:
			state = MRP_STATE_QP;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_LV:
		switch (state) {
		case MRP_STATE_VP:
			state = MRP_STATE_VO;
			break;
		case MRP_STATE_VN:
		case MRP_STATE_AN:
		case MRP_STATE_AA:
		case MRP_STATE_QA:
			state = MRP_STATE_LA;
			break;
		case MRP_STATE_AP:
			state = MRP_STATE_AO;
			break;
		case MRP_STATE_QP:
			state = MRP_STATE_QO;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_RNEW:
		break;

	case MRP_EVENT_RJOININ:
		switch (state) {
		case MRP_STATE_VO:
			if (!(app->participant_type & MRP_PARTICIPANT_TYPE_POINT_TO_POINT)) /* 802.1Q Table 10.3 Note 3 */
				state = MRP_STATE_AO;
			break;
		case MRP_STATE_VP:
			if (!(app->participant_type & MRP_PARTICIPANT_TYPE_POINT_TO_POINT)) /* 802.1Q Table 10.3 Note 3 */
				state = MRP_STATE_AP;
			break;
		case MRP_STATE_AA:
			state = MRP_STATE_QA;
			break;
		case MRP_STATE_AO:
			state = MRP_STATE_QO;
			break;
		case MRP_STATE_AP:
			state = MRP_STATE_QP;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_RIN:
		switch (state) {
		case MRP_STATE_AA:
			state = MRP_STATE_QA;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_RJOINMT:
	case MRP_EVENT_RMT:
		switch (state) {
		case MRP_STATE_QA:
			state = MRP_STATE_AA;
			break;
		case MRP_STATE_QO:
			state = MRP_STATE_AO;
			break;
		case MRP_STATE_QP:
			state = MRP_STATE_AP;
			break;
		case MRP_STATE_LO:
			state = MRP_STATE_VO;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_RLV:
	case MRP_EVENT_RLA:
	case MRP_EVENT_REDECLARE:
		switch (state) {
		case MRP_STATE_VO:
			state = MRP_STATE_LO;
			break;
		case MRP_STATE_AN:
			state = MRP_STATE_VN;
			break;
		case MRP_STATE_AA:
		case MRP_STATE_QA:
		case MRP_STATE_AP:
		case MRP_STATE_QP:
			state = MRP_STATE_VP;
			break;
		case MRP_STATE_AO:
		case MRP_STATE_QO:
			state = MRP_STATE_LO;
			break;
		default:
			break;
		}

		break;

	case MRP_EVENT_PERIODIC:
		switch (state) {
		case MRP_STATE_QA:
			state = MRP_STATE_AA;
			break;
		case MRP_STATE_QP:
			state = MRP_STATE_AP;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_TX:
		switch (state) {
		case MRP_STATE_VO:
			action = MRP_ACTION_S_;
			break;
		case MRP_STATE_VP:
			state = MRP_STATE_AA;
			action = MRP_ACTION_SJ;
			break;
		case MRP_STATE_VN:
			state = MRP_STATE_AN;
			action = MRP_ACTION_SN;
			break;
		case MRP_STATE_AN:
			if (registrar->state == MRP_STATE_IN) /* 802.1Q Table 10.3 Note 8 */
				state = MRP_STATE_QA;
			else
				state= MRP_STATE_AA;

			action = MRP_ACTION_SN;
			break;
		case MRP_STATE_AP:
		case MRP_STATE_AA:
			state = MRP_STATE_QA;
			action = MRP_ACTION_SJ;
			break;
		case MRP_STATE_QA:
			action = MRP_ACTION_SJ_;
			break;
		case MRP_STATE_LA:
			state = MRP_STATE_VO;
			action = MRP_ACTION_SL;
			break;
		case MRP_STATE_AO:
		case MRP_STATE_QO:
		case MRP_STATE_QP:
			action = MRP_ACTION_S_;
			break;
		case MRP_STATE_LO:
			state = MRP_STATE_VO;
			action = MRP_ACTION_S;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_TXLA:
		switch (state) {
		case MRP_STATE_VO:
			state = MRP_STATE_LO;
			action = MRP_ACTION_S_;
			break;
		case MRP_STATE_VP:
			state = MRP_STATE_AA;
			action = MRP_ACTION_S;
			break;
		case MRP_STATE_VN:
			state = MRP_STATE_AN;
			action = MRP_ACTION_SN;
			break;
		case MRP_STATE_AN:
			state = MRP_STATE_QA;
			action = MRP_ACTION_SN;
			break;
		case MRP_STATE_AA:
		case MRP_STATE_AP:
		case MRP_STATE_QP:
			state = MRP_STATE_QA;
			action = MRP_ACTION_SJ;
			break;
		case MRP_STATE_QA:
			action = MRP_ACTION_SJ;
			break;
		case MRP_STATE_LA:
		case MRP_STATE_AO:
		case MRP_STATE_QO:
			state = MRP_STATE_LO;
			action = MRP_ACTION_S_;
			break;
		case MRP_STATE_LO:
			state = MRP_STATE_VO;
			action = MRP_ACTION_S_;
			break;
		default:
			break;
		}
		break;

	case MRP_EVENT_TXLAF:
		switch (state) {
		case MRP_STATE_VO:
			state = MRP_STATE_LO;
			break;
		case MRP_STATE_VP:
		case MRP_STATE_VN:
		case MRP_STATE_LO:
			break;
		case MRP_STATE_AN:
			state = MRP_STATE_VN;
			break;

		case MRP_STATE_AA:
		case MRP_STATE_QA:
		case MRP_STATE_AP:
		case MRP_STATE_QP:
			state = MRP_STATE_VP;
			break;
		case MRP_STATE_LA:
		case MRP_STATE_AO:
		case MRP_STATE_QO:
			state = MRP_STATE_LO;
			break;
		default:
			break;
		}
		break;

	default:
		os_log(LOG_ERR, "invalid event(%d, %s)\n", event, mrp_event2string(event));
		break;
	}

	/* 802.1Q - Table 10.3, Note 1 */
	if (!(app->participant_type & MRP_PARTICIPANT_TYPE_FULL))
		if (state == MRP_STATE_LO)
			state = MRP_STATE_VO;

	/* 802.1Q - Table 10.3, Note 6 */
	if ((state != applicant->state) && ((state == MRP_STATE_VN) || (state == MRP_STATE_AN) || (state == MRP_STATE_AA) ||
	    (state == MRP_STATE_LA) || (state == MRP_STATE_VP) || (state == MRP_STATE_AP) ||
	    (state == MRP_STATE_LO)))
		mrp_transmit_request(app);

#if defined (CFG_MRP_SM_LOG)
	os_log(LOG_DEBUG, "(%p, %p) event(%s) state from %s to %s action(%s)\n", app, attr,
	       mrp_event2string(event), mrp_state2string(applicant->state), mrp_state2string(state), mrp_action2string(action));
#endif

	if (state != applicant->state) {
		applicant->state = state;

		list_del(&attr->list_ordered);
		list_add_tail(&app->attributes_ordered[attr->type], &attr->list_ordered);
	}

	applicant->action = action;

	mrp_applicant_action_handler(app, attr);
}



/** Perform appropriate action depending on registrar state machine output
 * \return	none
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 * \param attr	pointer to the MRP attribute the registrar state machine has been run for
 */
static void mrp_registrar_action_handler(struct mrp_application *app, struct mrp_attribute *attr)
{
	switch (attr->registrar.action) {
	case MRP_ACTION_NEW:
		app->mad_join_indication(app, attr, true);
		break;

	case MRP_ACTION_JOIN:
		app->mad_join_indication(app, attr, false);
		break;

	case MRP_ACTION_LV:
		app->mad_leave_indication(app, attr);
		break;

	default:
		break;
	}
}


/** MRP registrar state machine (802.1Q - Table 10.4)
 * The job of the Registrar is to record declarations of the attribute made by others applicant on the LAN. It does not send any protocol message
 * \return	none
 * \param attr	pointer to the MRP attribute the registrar state machine is updated for
 * \param event	mrp event value. Can originate from timers, received PDU or transmitted declaration
 */
static void mrp_registrar_sm(struct mrp_attribute *attr, mrp_protocol_event_t event)
{
	struct mrp_application *app = attr->app;
	struct mrp_registrar *registrar = &attr->registrar;
	mrp_protocol_state_t state = registrar->state;
	mrp_protocol_action_t action = MRP_ACTION_NONE;
	bool immediate = false;

	if (!(app->participant_type & MRP_PARTICIPANT_TYPE_FULL))
		goto exit_not_enabled;

begin:
	switch (event) {
	case MRP_EVENT_BEGIN:
		state = MRP_STATE_MT;
		break;

	case MRP_EVENT_RNEW:
		action = MRP_ACTION_NEW;

		if (state == MRP_STATE_LV)
			timer_stop(&registrar->leave.timer);

		state = MRP_STATE_IN;
		break;

	case MRP_EVENT_RJOININ:
	case MRP_EVENT_RJOINMT:
		if (state == MRP_STATE_LV)
			timer_stop(&registrar->leave.timer);
		else if (state == MRP_STATE_MT)
			action = MRP_ACTION_JOIN;

		state = MRP_STATE_IN;
		break;

	case MRP_EVENT_RLV:
		/* Per Milan spec 1.2 (4.2.7.2.2: Instantaneous transition from IN to MT)
		 * Per 802.1Qcc-2018 (10.7.8 Table 10-4: Registrar state table)
		 */
		if (state == MRP_STATE_IN) {
			action = MRP_ACTION_LV;
			state = MRP_STATE_MT;
			break;
		}
		/* fallthrough */
	case MRP_EVENT_RLA:
	case MRP_EVENT_TXLA:
	case MRP_EVENT_REDECLARE:
		if (state == MRP_STATE_IN) {
			timer_start(&registrar->leave.timer, app->srp->port[app->port_id].mrp.leave_timeout);

			state = MRP_STATE_LV;
		}
	#if 0
		else
			os_log(LOG_DEBUG, "event(%s) invalid for state(%s)\n", mrp_event2string(event), mrp_state2string(state));
	#endif
		break;

	case MRP_EVENT_RLV_IMMEDIATE:
		if ((state == MRP_STATE_IN) || (state == MRP_STATE_LV)) {
			immediate = true;

			event = MRP_EVENT_LEAVETIMER;

			state = MRP_STATE_LV;

			goto begin;
		}

		break;

	case MRP_EVENT_FLUSH:
		if (state == MRP_STATE_LV)
			action = MRP_ACTION_LV;

		state = MRP_STATE_MT;
		break;

	case MRP_EVENT_LEAVETIMER:
		if (state == MRP_STATE_LV) {
			if (!immediate)
				action = MRP_ACTION_LV;

			state = MRP_STATE_MT;
		} else if (state == MRP_STATE_MT)
			state = MRP_STATE_MT;
	#if 0
		else
			os_log(LOG_DEBUG, "event(%s) invalid for state(%s)\n", mrp_event2string(event), mrp_state2string(state));
	#endif
		break;

	default:
		os_log(LOG_DEBUG, "event(%s) invalid\n", mrp_event2string(event));
		break;
	}

#if defined (CFG_MRP_SM_LOG)
	os_log(LOG_DEBUG, "(%p, %p) event(%s) state from %s to %s action(%s)\n", app, attr, mrp_event2string(event),
	       mrp_state2string(registrar->state), mrp_state2string(state), mrp_action2string(action));
#endif

exit_not_enabled:
	registrar->state = state;
	registrar->action = action;

	mrp_registrar_action_handler(app, attr);
}

/** Leave timer timeout handler. Used by registrar only and per attribute
 * \return	void
 * \param data	pointer to the MRP attribute whose leave timer timed out
 */
static void mrp_leave_timer_handler(void *data)
{
	struct mrp_attribute *attr = (struct mrp_attribute *)data;

	mrp_registrar_sm(attr, MRP_EVENT_LEAVETIMER);

	mrp_free_attribute_check(attr->app, attr);
}



/** Perform appropriate action (send leaveall PDU) on leave all state machine output
 * \return	none
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 */
static void mrp_leaveall_action_handler(struct mrp_application *app)
{
	struct mrp_attribute *attr;
	struct list_head *entry, *next;
	unsigned int type;

	switch (app->leaveall.action) {
	case MRP_ACTION_SLA:

		for (type = app->min_attr_type; type <= app->max_attr_type; type++)
			for (entry = list_first(&app->attributes[type]); next = list_next(entry), entry != &app->attributes[type]; entry = next) {

				attr = container_of(entry, struct mrp_attribute, list);

				/* 802.1Q - 10.7.9
				LeaveAll messages generated by this state machine also generate LeaveAll events against all the Applicant and
				Registrar state machines associated with that applicant and port
				*/

				__mrp_process_attribute(attr, MRP_ATTR_EVT_LVA);
			}

		for (type = app->min_attr_type; type <= app->max_attr_type; type++)
			mrp_transmit_action(app, type, NULL, 0, 1);

		break;

	default:
		break;
	}
}



/** MRP leaveAll state machine (start timer, send leaveall PDU)
 * 802.1Q - Table 10.5 A single LeaveAll state machine exists for each full MRP Participant.
 * Leave All messages generated by this state machine also generate LeaveAll events against all the Applicant and Registrar state machines
 * associated with that Participant and Port; hence, LeaveAll generation is treated by those state machines in
 * the same way as reception of a LeaveAll message from an external source.
 * \return	none
 * \param app	pointer to the MRP application the state machine is executed for
 * \param event	MRP protocol event value
 */
static void mrp_leaveall_sm(struct mrp_application *app, mrp_protocol_event_t event)
{
	struct mrp_leaveall *lva = &app->leaveall;
	mrp_protocol_timer_state_t state = lva->state;
	mrp_protocol_action_t action = MRP_ACTION_NONE;

	if (!(app->participant_type & MRP_PARTICIPANT_TYPE_FULL))
		goto exit_not_enabled;

	switch (event) {
	case MRP_EVENT_BEGIN:
		state = MRP_TIMER_PASSIVE;
		timer_start(&lva->timer, random_range(MRP_LVATIMER_VAL_MIN, MRP_LVATIMER_VAL_MAX));
		break;

	case MRP_EVENT_TX:
		if (state == MRP_TIMER_ACTIVE) {
			state = MRP_TIMER_PASSIVE;
			action = MRP_ACTION_SLA;
		}
	#if 0
		else
			os_log(LOG_DEBUG, "event(%s) invalid for state(%s)\n", mrp_event2string(event), mrp_timer_state2string(state));
	#endif
		break;

	case MRP_EVENT_RLA:
		state = MRP_TIMER_PASSIVE;
		timer_restart(&lva->timer, random_range(MRP_LVATIMER_VAL_MIN, MRP_LVATIMER_VAL_MAX));
		break;

	case MRP_EVENT_LEAVEALLTIMER:
		state = MRP_TIMER_ACTIVE;

		mrp_transmit_request(app);

		timer_start(&lva->timer, random_range(MRP_LVATIMER_VAL_MIN, MRP_LVATIMER_VAL_MAX));
		break;

	default:
		os_log(LOG_DEBUG, "event(%s) invalid\n", mrp_event2string(event));
		break;
	}

#if defined (CFG_MRP_SM_LOG)
	os_log(LOG_DEBUG, "(%p) event(%s) state from %s to %s action(%s)\n", app, mrp_event2string(event),
	       mrp_timer_state2string(lva->state), mrp_timer_state2string(state), mrp_action2string(action));
#endif

exit_not_enabled:
	lva->state = state;
	lva->action = action;

	mrp_leaveall_action_handler(app);
}


/** LeaveAll timer timeout handler. Simply restart the timer
 * \return	none
 * \param data	pointer to the MRP application (MVRP, MSRP, MMRP)
 */
static void mrp_leaveall_timer_handler(void *data)
{
	struct mrp_application *app = (struct mrp_application *)data;

	os_log(LOG_DEBUG, "mrp_app(%p)\n", app);

	/* restart the timer for the whole application */
	mrp_leaveall_sm(app, MRP_EVENT_LEAVEALLTIMER);
}


/** Join timer handler. Common to all MRP attributes.
 * \return	none
 * \param data	pointer to the MRP application (MVRP, MSRP, MMRP)
 */
static void mrp_join_timer_handler(void *data)
{
	struct mrp_application *app = (struct mrp_application *)data;
	struct list_head *entry, *next, *last;
	struct mrp_attribute *attr;
	mrp_protocol_event_t event = MRP_EVENT_TX;
	unsigned int type;

	mrp_leaveall_sm(app, MRP_EVENT_TX);
	if (app->leaveall.action == MRP_ACTION_SLA)
		event = MRP_EVENT_TXLA;

	for (type = app->min_attr_type; type <= app->max_attr_type; type++) {
		/* In case of an empty list, don't process head */
		if (list_empty(&app->attributes_ordered[type]))
			continue;

		/* mrp_applicant_sm() can reorder the attributes_ordered list and push the current attribute to the tail.
		 * Make sure to start from first of the list and stop at the initial last attribute.
		 */
		next = list_first(&app->attributes_ordered[type]);
		last = list_last(&app->attributes_ordered[type]);

		do {
			entry = next;
			next = list_next(entry);

			attr = container_of(entry, struct mrp_attribute, list_ordered);

			mrp_applicant_sm(attr, event);

			mrp_free_attribute_check(app, attr);

		} while (entry != last);
	}

	for (type = app->min_attr_type; type <= app->max_attr_type; type++)
		mrp_transmit_end(app, type);
}


/** A single PeriodicTransmission state machine exists for each Port. Periodic Transmission events are
 * generated on a regular basis, against all Applicant state machines that are associated with that Port.802.1Q - Table 10.6
 * \return	none
 * \param app	pointer to the MRP application (MVRP, MSRP, MMRP)
 * \param event	MRP protocol event value
 */
static void mrp_periodic_sm(struct mrp_application *app, mrp_protocol_event_t event)
{
	struct mrp_periodic *periodic = &app->periodic;
	mrp_protocol_timer_state_t state = periodic->state;
	mrp_protocol_action_t action = MRP_ACTION_NONE;

	switch (event) {
	case MRP_EVENT_BEGIN:
		state = MRP_TIMER_ACTIVE;
		timer_start(&periodic->timer, MRP_PERIODTIMER_VAL);
		break;

	case MRP_EVENT_PERIODIC_ENABLED:
		if (state == MRP_TIMER_PASSIVE) {
			state = MRP_TIMER_ACTIVE;
			timer_start(&periodic->timer, MRP_PERIODTIMER_VAL);
		} else
			os_log(LOG_ERR, "event(%s) invalid for state(%s)\n", mrp_event2string(event), mrp_timer_state2string(state));

		break;

	case MRP_EVENT_PERIODIC_DISABLED:
		if (state == MRP_TIMER_ACTIVE) {
			state = MRP_TIMER_PASSIVE;
			timer_stop(&periodic->timer); /* FIXME this is not mentioned in the spec... */
		} else
			os_log(LOG_ERR, "event(%s) invalid for state(%s)\n", mrp_event2string(event), mrp_timer_state2string(state));

		break;

	case MRP_EVENT_PERIODICTIMER:
		if (state == MRP_TIMER_ACTIVE) {
			action = MRP_ACTION_PERIODIC;
			timer_start(&periodic->timer, MRP_PERIODTIMER_VAL);
		} else
			os_log(LOG_ERR, "event(%s) invalid for state(%s)\n", mrp_event2string(event), mrp_timer_state2string(state));

		break;

	default:
		os_log(LOG_ERR, "event(%s) invalid\n", mrp_event2string(event));
		break;
	}

#if defined (CFG_MRP_SM_LOG)
	//os_log(LOG_DEBUG, "(%p) event(%s) state from %s to %s action(%s)\n", app, mrp_event2string(event),
	//	mrp_timer_state2string(periodic->state), mrp_timer_state2string(state), mrp_action2string(action));
#endif

	periodic->state = state;
	periodic->action = action;
}


/** Periodic timer handler. Common to all MRP attributes
 * \return	none
 * \param data	pointer to the MRP application (MSRP, MVRP, MMRP)
 */
static void mrp_periodic_timer_handler(void *data)
{
	struct mrp_application *app = (struct mrp_application *)data;
	struct list_head *entry, *next;
	struct mrp_attribute *attr;
	unsigned int type;

	for (type = app->min_attr_type; type <= app->max_attr_type; type++)
		for (entry = list_first(&app->attributes[type]); next = list_next(entry), entry != &app->attributes[type]; entry = next) {

			attr = container_of(entry, struct mrp_attribute, list);

			mrp_applicant_sm(attr, MRP_EVENT_PERIODIC);
		}

	/* restart the timer */
	mrp_periodic_sm(app, MRP_EVENT_PERIODICTIMER);
}


/*
An MRP Participant consists of an application component, and an MRP Attribute Declaration (MAD)
component. The application component is responsible for the semantics associated with Attribute values and
their registration, including the use of explicitly signaled new declarations, and uses the following two
primitives to request MAD to make or withdraw Attribute declarations:
*/

/** Send MRP join request for a given attribute (802.1Q - 10.2)
 * \return	pointer to the concerned mrp_attribute
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 * \param type	MRP attribute's type
 * \param val	MRP attribute's values
 * \param new	boolean, indicates an explicit new declaration
 */
struct mrp_attribute *mrp_mad_join_request(struct mrp_application *app, unsigned int type, u8 *val, bool new)
{
	struct mrp_attribute *attr;

	if (!app->enabled)
		goto err;

	attr = mrp_get_attribute(app, type, val);
	if (!attr)
		goto err;

	os_log(LOG_INFO, "mrp_app(%p) port(%u) attr(%p, %s) new(%d)\n", app, app->port_id, attr, app->attribute_type_to_string(attr->type), new);

	if (new)
		mrp_applicant_sm(attr, MRP_EVENT_NEW);
	else
		mrp_applicant_sm(attr, MRP_EVENT_JOIN);

	return attr;

err:
	return NULL;
}

/** Send MRP join immediately, without waiting the next transmission opportunity, for a given attribute.
 * This is not specified in 802.1Q but would be needed to support direct change of attribute declaration for MSRP (802.1Q-2022 35.2.6 Encoding)
 * On a change of declaration (for previously declared MSRP attribute), the applicant needs to send the join immediately to avoid contention
 * between the JoinTimer for the TX and the upper layer issuing a mad_leave_request(), causing the attribute to be freed without any PDU transmission.
 * \return	pointer to the concerned mrp_attribute
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 * \param type	MRP attribute's type
 * \param val	MRP attribute's values
 */
struct mrp_attribute *mrp_mad_join_immediate(struct mrp_application *app, unsigned int type, u8 *val)
{
	struct mrp_attribute *attr;

	if (!app->enabled)
		goto err;

	/* Only valid if the attribute is new */
	attr = mrp_find_attribute(app, type, val);
	if (attr)
		goto err;

	attr = mrp_alloc_attribute(app, type, val);
	if (!attr)
		goto err;

	os_log(LOG_INFO, "mrp_app(%p) port(%u) attr(%p, %s)\n", app, app->port_id, attr, app->attribute_type_to_string(attr->type));

	mrp_applicant_sm(attr, MRP_EVENT_JOIN);
	mrp_applicant_sm(attr, MRP_EVENT_TX);

	return attr;

err:
	return NULL;
}

/** Send MRP leave request for a given MRP attribute (802.1Q - 10.2)
 * \return	none
 * \param app	pointer to the MRP application (MSRP, MVRP, MMRP)
 * \param attr	pointer to the MRP attribute
 */
void mrp_mad_leave_request(struct mrp_application *app, unsigned int type, u8 *val)
{
	struct mrp_attribute *attr;

	if (!app->enabled)
		goto out;

	attr = mrp_find_attribute(app, type, val);
	if (attr) {
		os_log(LOG_INFO, "mrp_app(%p) port(%u) attr(%p, %s)\n", app, app->port_id, attr, app->attribute_type_to_string(attr->type));

		mrp_applicant_sm(attr, MRP_EVENT_LV);

		mrp_free_attribute_check(app, attr);
	}

out:
	return;
}


/** Map MRP received PDU event into state machines events
 * \return	0 on success, negative value on failure
 * \param attr	pointer to the MRP attribute
 * \param event	MRP protocol attribute event value (NEW, JOININ, IN...)
 */
static int __mrp_process_attribute(struct mrp_attribute *attr, mrp_protocol_attribute_event_t event)
{
	struct mrp_application *app = attr->app;

	if (event >= MRP_ATTR_EVT_MAX) {
		os_log(LOG_ERR, "attr(%p, %s) event(%d) out of range\n", attr, app->attribute_type_to_string(attr->type), event);
		return -1;
	}

	/* Some applicant actions depend on registrar state, process registrar state machine first */
	mrp_registrar_sm(attr, mrp_attribute2rxevent[event]);
	mrp_applicant_sm(attr, mrp_attribute2rxevent[event]);

	mrp_free_attribute_check(app, attr);

	return 0;
}

int mrp_process_attribute(struct mrp_application *app, unsigned int type, u8 *val, mrp_protocol_attribute_event_t event)
{
	struct mrp_attribute *attr;

	if (!app->enabled)
		goto err;

	attr = mrp_get_attribute(app, type, val);
	if (attr)
		return __mrp_process_attribute(attr, event);

err:
	return -1;
}

void mrp_process_attribute_free_immediate(struct mrp_attribute *attr)
{
	struct mrp_application *app = attr->app;

	os_log(LOG_INFO, "attr(%p, %s)\n", attr, app->attribute_type_to_string(attr->type));

	mrp_free_attribute(attr);
}

int mrp_process_attribute_leave_immediate(struct mrp_attribute *attr)
{
	struct mrp_application *app = attr->app;

	os_log(LOG_INFO, "attr(%p, %s)\n", attr, app->attribute_type_to_string(attr->type));

	/* Some applicant actions depend on registrar state, process registrar state machine first */
	mrp_registrar_sm(attr, MRP_EVENT_RLV_IMMEDIATE);
	mrp_applicant_sm(attr, MRP_EVENT_RLV);

	mrp_free_attribute_check(app, attr);

	return 0;
}

int mrp_process_packet(struct mrp_application *app, struct net_rx_desc *desc)
{
	struct eth_hdr *eth = (struct eth_hdr *)((u8 *)desc + desc->l2_offset);
	u8 *data = (u8 *)desc + desc->l3_offset;
	u8 *end = (u8 *)NET_DATA_START(desc) + desc->len;
	struct mrp_pdu_header *mrp_header;
	u16 *vector_header, *vector_end_mark;
	u8 *vector_end;
	unsigned int vector_length;
	unsigned int type;
	unsigned int leave_all_event, number_of_values;
	unsigned int newer_version = 0, skip;
	struct list_head *entry;
	struct mrp_attribute *attr;

	if (!app->enabled)
		goto err;

	if (os_memcmp(app->dst_mac, eth->dst, 6))
		goto err;

	app->num_rx_pkts++;

	if (*data < app->proto_version) {
		os_log(LOG_ERR, "mrp_app(%p) unsupported/older protocol version %u\n", app, *data);
		goto err;
	}

	if (*data > app->proto_version) {
		os_log(LOG_DEBUG, "mrp_app(%p) newer protocol version %u\n", app, *data);
		newer_version = 1;
	}

	/* mrp_get_first_message returns NULL if whole length of the pdu is shorter than a mrp header size */
	mrp_header = mrp_get_first_message(app, data, end);

	/* we have to go all over the pdu until end marker if found */
	while (mrp_header) {
		skip = 0;

		if (app->attribute_check(mrp_header) < 0) {
			/* Invalid attribute */
			if (newer_version) {
				/* 802.1Q-2011, 10.8.3.5, skip entire message */
				skip = 1;
			} else
				goto err;
		}

		vector_header = mrp_get_first_vector(app, mrp_header);

		if (app->has_attribute_list_length) {
			vector_end = (u8*)vector_header + ntohs(mrp_header->attribute_list_length);

			if (skip) {
				vector_header = (u16 *)(vector_end - 2);
				goto skip_message;
			}

			/* check attribute list fits within the pdu size */
			if (vector_end > end) {
				os_log(LOG_ERR, "mrp_app(%p) invalid vector, doesn't fit in packet\n", app);
				goto err;
			}
		} else
			vector_end = end;

		do {
			/* Check if vector can contain header */
			if ((u8 *)(vector_header + 1) > vector_end) {
				os_log(LOG_ERR, "mrp_app(%p) corrupted vector\n", app);
				goto err;
			}

			mrp_get_vector_header(*vector_header, &leave_all_event, &number_of_values);

			vector_length = sizeof(*vector_header) + mrp_header->attribute_length + app->event_length(mrp_header, number_of_values);
			if ((u8 *)vector_header + vector_length > vector_end) {
				os_log(LOG_ERR, "mrp_app(%p) truncated vector\n", app);
				goto err;
			}

			if (!skip) {
				/* 802.1Q-2011, 10.8.2.6 */
				if ((leave_all_event != MRP_NULL_LVA_EVENT) && (leave_all_event != MRP_LVA_EVENT)) {
					os_log(LOG_ERR, "mrp_app(%p) invalid leave all event %d\n", app, leave_all_event);
					goto err;
				}

				/* check for malformed packet */
				if (!number_of_values && (leave_all_event == MRP_NULL_LVA_EVENT)) {
					os_log(LOG_ERR, "mrp_app(%p) malformed packet number_of_values = 0\n", app);
					goto err;
				}

				os_log(LOG_DEBUG, "mrp_app(%p) leave_all_event %u number_of_values %u\n", app, leave_all_event, number_of_values);

				if (leave_all_event) {
					type = mrp_header->attribute_type;

					for (entry = list_first(&app->attributes[type]); entry != &app->attributes[type]; entry = list_next(entry)) {
						attr = container_of(entry, struct mrp_attribute, list);

						__mrp_process_attribute(attr, MRP_ATTR_EVT_LVA);
					}

					mrp_leaveall_sm(app, MRP_EVENT_RLA);
				}

				app->vector_handler(app, mrp_header, vector_header + 1, number_of_values);
			}

			vector_header = (u16 *)((u8 *)vector_header + vector_length);
		} while (*vector_header);

	skip_message:
		vector_end_mark = vector_header;

		mrp_header = mrp_get_next_message(vector_end_mark, end);
	}

	return 0;

err:
	return -1;
}

/** Allocates MRP attribute and initializes states machines
 * \return	0 on success, negative value on failure
 * \param app	pointer to the MRP application the attribute belongs to (MSRP, MVRP, MMRP)
 * \param type	attribute type, application dependent
 * \param val	attribute value, attribute type dependent
 */
static struct mrp_attribute *mrp_alloc_attribute(struct mrp_application *app, unsigned int type, u8 *val)
{
	struct mrp_attribute *attr;
	unsigned int len = app->attribute_value_length(type);

	attr = os_malloc(sizeof(struct mrp_attribute) + len);
	if (!attr)
		goto err_alloc;

	attr->app = app;
	attr->type = type;

	attr->registrar.leave.timer.func = mrp_leave_timer_handler;
	attr->registrar.leave.timer.data = attr;

	os_memcpy(attr->val, val, len);

	list_add(&app->attributes[type], &attr->list);
	list_add(&app->attributes_ordered[type], &attr->list_ordered);

	if (timer_create(app->srp->timer_ctx, &attr->registrar.leave.timer, 0, app->srp->port[app->port_id].mrp.leave_timeout) < 0)
		goto err_timer;

	mrp_applicant_sm(attr, MRP_EVENT_BEGIN);
	mrp_registrar_sm(attr, MRP_EVENT_BEGIN);

	os_log(LOG_INFO, "mrp_app(%p) port(%u) attr(%p, %s)\n", app, app->port_id, attr, app->attribute_type_to_string(attr->type));

	return attr;

err_timer:
	os_free(attr);

err_alloc:
	return NULL;
}

/** Finds MRP attribute
 * \return	pointer to attribute on success, NULL on failure
 * \param app	pointer to the MRP application the attribute belongs to (MSRP, MVRP, MMRP)
 * \param type	attribute type, application dependent
 * \param val	attribute value, attribute type dependent
 */
static struct mrp_attribute *mrp_find_attribute(struct mrp_application *app, unsigned int type, u8 *val)
{
	struct list_head *entry;
	struct mrp_attribute *attr;
	unsigned int len = app->attribute_value_length(type);

	for (entry = list_first(&app->attributes[type]); entry != &app->attributes[type]; entry = list_next(entry)) {
		attr = container_of(entry, struct mrp_attribute, list);

		if (os_memcmp(attr->val, val, len))
			continue;

		return attr;
	}

	return NULL;
}

/** Gets MRP attribute, if attribute doesn't exist it is created
 * \return	pointer to attribute on success, NULL on failure
 * \param app	pointer to the MRP application the attribute belongs to (MSRP, MVRP, MMRP)
 * \param type	attribute type, application dependent
 * \param val	attribute value, attribute type dependent
 */
static struct mrp_attribute *mrp_get_attribute(struct mrp_application *app, unsigned int type, u8 *val)
{
	struct mrp_attribute *attr;

	attr = mrp_find_attribute(app, type, val);
	if (!attr)
		attr = mrp_alloc_attribute(app, type, val);

	return attr;
}

/** Free attribute, and destroy associated timers
 * \return	none
 * \param attr	pointer to the MRP attribute
 */
static void mrp_free_attribute(struct mrp_attribute *attr)
{
	struct mrp_application *app = attr->app;

	os_log(LOG_INFO, "mrp_app(%p) port(%u) attr(%p, %s)\n", app, app->port_id, attr, app->attribute_type_to_string(attr->type));

	timer_destroy(&attr->registrar.leave.timer);

	list_del(&attr->list);
	list_del(&attr->list_ordered);

	os_free(attr);
}

/** Exits all MRP attributes.
 * The MRP application is supposed to have called mrp_mad_leave_request() for all attributes previously.
 * \return	none
 * \param app	pointer to the MRP application  (MSRP, MVRP, MMRP)
 */
__init static void mrp_exit_attributes(struct mrp_application *app)
{
	struct list_head *entry, *entry_next;
	struct mrp_attribute *attr;
	unsigned int type;

	for (type = 0; type < MRP_MAX_ATTR_TYPE; type++)
		for (entry = list_first(&app->attributes[type]); entry_next = list_next(entry), entry != &app->attributes[type]; entry = entry_next) {
			attr = container_of(entry, struct mrp_attribute, list);

			mrp_free_attribute(attr);
		}
}

/**
 * MRP application level enable. Starts MRP state machines.
 * \param app	pointer to the MRP application context
 */
__init void mrp_enable(struct mrp_application *app)
{
	if (app->enabled)
		goto out;

	/* FIXME: to be confirmed if all timers and required for all kind of application, for now
	we see some differents description in specs and AVnu UT */

	/* FIXME
	 * Periodic seems to be required by all applicants but can be disabled by the user and for MSRP is not used (802.1Q - 5.16.3)
	 * Leave is only used by registrar (and is started by registrar state machine)
	 * LeaveAll seems to only be used by full participants
	 * Join is used always used and is started on transmit requests */

	if (app->participant_type & MRP_PARTICIPANT_TYPE_FULL)
		mrp_leaveall_sm(app, MRP_EVENT_BEGIN);

	if (app->type != APP_MSRP) {
		mrp_periodic_sm(app, MRP_EVENT_BEGIN);

		/* keep it disabled by default */
//		mrp_periodic_sm(app, MRP_EVENT_PERIODIC_DISABLED);
	}

	app->enabled = true;

out:
	return;
}

/**
 * MRP application level disable. Frees all attributes and stops all state machines.
 * \param app	pointer to the MRP application context
 */
__exit void mrp_disable(struct mrp_application *app)
{
	if (!app->enabled)
		goto out;

	if (app->type != APP_MSRP)
		timer_stop(&app->periodic.timer);

	timer_stop(&app->join.timer);
	timer_stop(&app->leaveall.timer);

	mrp_exit_attributes(app);

	app->enabled = false;

out:
	return;
}

/** Create and initialize MRP timers for a given application
 * \return	0 on success, negative value on failure
 * \param app	pointer to the MRP application  (MSRP, MVRP, MMRP)
 */
__init static int mrp_init_timers(struct mrp_application *app)
{
	app->leaveall.timer.func = mrp_leaveall_timer_handler;
	app->leaveall.timer.data = app;

	if (timer_create(app->srp->timer_ctx, &app->leaveall.timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_timer_leaveall;

	/* The Join Period Timer controls the interval between transmission opportunities that are applied to the applicant state machine */
	app->join.timer.func = mrp_join_timer_handler;
	app->join.timer.data = app;

	if (timer_create(app->srp->timer_ctx, &app->join.timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_timer_join;

	if (app->type != APP_MSRP) {
		/* The Periodic Transmission Timer controls the frequency with which the Periodic Transmission state machine generates periodic events */
		app->periodic.timer.func = mrp_periodic_timer_handler;
		app->periodic.timer.data = app;

		if (timer_create(app->srp->timer_ctx, &app->periodic.timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_timer_periodic;
	}

	return 0;

err_timer_periodic:
	timer_destroy(&app->join.timer);

err_timer_join:
	timer_destroy(&app->leaveall.timer);

err_timer_leaveall:
	return -1;
}


/** Destroy MRP timers for a given application
 * \return	none
 * \param app	pointer to the MRP application  (MSRP, MVRP, MMRP)
 */
__exit static void mrp_exit_timers(struct mrp_application *app)
{
	if (app->type != APP_MSRP)
		timer_destroy(&app->periodic.timer);

	timer_destroy(&app->join.timer);
	timer_destroy(&app->leaveall.timer);
}

/**
 * Initialize the MRP application level states machines (called once for each MRP application)
 * \return	0 on success, negative value on failure
 * \param app	pointer to the MRP application context
 * \param type	type of the MRP application  (MSRP, MVRP, MMRP)
 * \param participant_type specify if the application is acting as a full participant or applicant only
 */
__init int mrp_init(struct mrp_application *app, unsigned int type, unsigned int participant_type)
{
	int i;

	/* The Leave All Period Timer controls the frequency with which the LeaveAll state machine generates LeaveAll PDUs */

	app->type = type;
	app->participant_type = participant_type;
	app->enabled = false;

	os_memset(app->mrpdu, 0, sizeof(app->mrpdu));

	if (mrp_init_timers(app) < 0)
		goto err_timers;

	for (i = 0; i < MRP_MAX_ATTR_TYPE; i++) {
		list_head_init(&app->attributes[i]);
		list_head_init(&app->attributes_ordered[i]);
	}

	mrp_enable(app);

	os_log(LOG_INIT, "mrp_app(%p) done\n", app);

	return 0;

err_timers:
	return -1;
}


/**
 * MRP application level clean-up (called once for each MRP application)
 * \return	0 on success, negative value on failure
 * \param app	pointer to the MRP application context
 */
__exit int mrp_exit(struct mrp_application *app)
{
	mrp_disable(app);

	mrp_exit_timers(app);

	os_log(LOG_INIT, "done\n");

	return 0;
}
