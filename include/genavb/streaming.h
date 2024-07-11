/*
 * Copyright 2018-2019, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details Streaming API definition for the GenAVB library

 \copyright Copyright 2018-2019, 2023-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_STREAMING_API_H_
#define _GENAVB_PUBLIC_STREAMING_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "avdecc.h"
#include "control_clock_domain.h"

/**
 * \ingroup stream
 */
typedef enum {
	GENAVB_STREAM_FLAGS_MCR = (1 << 0),	/**< Enable media clock recovery for the stream. Only valid for listener streams and ::GENAVB_MEDIA_CLOCK_DOMAIN_STREAM clock domain*/
	GENAVB_STREAM_FLAGS_CUSTOM_TSPEC = (1 << 1),	/**< Enable custom tspec definition. Only valid for talker streams */
	GENAVB_STREAM_FLAGS_MAX_TRANSIT_TIME_VALID = (1 << 2)	/**< Valid presentation_time_offset in stream params. Only valid for talker streams */
} genavb_stream_flags_t;

/**
 * \ingroup stream
 * genavb_stream_create flags
 */
typedef enum {
	AVTP_NONBLOCK = (1 << 0), /**< Create stream in non-blocking mode */
	AVTP_DGRAM = (1 << 1)	/**< Create stream in DATAGRAM mode */
} genavb_stream_create_flags_t;


/**
 * \ingroup stream
 * Stream creation parameters
 */
struct genavb_stream_params {
	avtp_direction_t direction;		/**< Stream direction */
	avb_u8 subtype;				/**< Stream subtype (1722-2011 Table 5.2, 1722-2016 Table 6) */
	avb_u16 port;				/**< Network port */
	sr_class_t stream_class;		/**< Stream class */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
	avb_u8 dst_mac[6];			/**< Stream destination mac address (in network order), needs to be specified for both listener and talker cases */
	struct avdecc_format format;		/**< Stream media format (in network order) */
	genavb_stream_flags_t flags;		/**< Stream flags bitmap */
	genavb_clock_domain_t clock_domain;	/**< Media clock domain used for this stream */

	struct {
		avb_u32 latency;		/**< Stream processing latency/period in ns. AVTP thread is woken periodically based on this setting to process pending media data for the stream.
							A low value reduces overall latency (between talker and listener), but increases CPU usage.
							A high value increases overall latency (between talker and listener), but decreases CPU usage.
							This setting is ignored for subtypes @ref AVTP_SUBTYPE_CRF, @ref AVTP_SUBTYPE_TSCF and @ref AVTP_SUBTYPE_NTSCF.

						In 1.x releases:
							- Internally the stack uses a minimum value of 500us.
							- The maximum value is such that no more than 16 packets per stream are generated per period.
							That works out to around 4ms for Class B streams and 2ms for Class A.
							- The value is adjusted to always have an integer number of transmitted packets per period.
							- All talker streams from the same SR class must be created with the same latency setting (or creation will fail).*/
		avb_u32 max_transit_time;	/**< If ::GENAVB_STREAM_FLAGS_MAX_TRANSIT_TIME_VALID is set in flags, this value is used as stream presentation offset (ns) for talker avtp timestamps.
							Otherwise, the default max_transit_time for the stream class is used. */
		avb_u16 vlan_id;		/**< Vlan id for the stream (in network order), one of [VLAN_VID_MIN, VLAN_VID_MAX], VLAN_VID_DEFAULT or VLAN_VID_NONE.
							For streams with a reservation, VLAN_VID_DEFAULT will use the vlan id of the SRP domain. VLAN_VID_NONE will result in a error. Any other value will override the SRP domain vlan id.
							For streams without a reservation, VLAN_VID_NONE/VLAN_VID_DEFAULT will result in frames with no vlan tag. Any other value will results in vlan tagged frames. */
		avb_u8 priority;		/**< (Vlan) priority for the stream, only used for streams without a reservation. Must not overlap with SRP domain priorities */

		avb_u16 max_frame_size;
		avb_u16 max_interval_frames;
	} talker; 				/**< Talker specific parameters */
};

/** Create a new AVTP stream.
 * \ingroup stream
 * \return 		::GENAVB_SUCCESS or negative error code. In case of success: stream argument is updated with stream handle, batch_size argument is updated with actual configured batch size.
 * \param genavb		pointer to GenAVB library handle
 * \param stream	pointer to stream handle pointer
 * \param params	pointer to struct containing all required params.
 * \param batch_size	requested batch size in bytes. The media stack will be woken up __only__ when the number of bytes free/available is equal or above batch size.
 *			After wakeup, the listener can always read batch size bytes and the talker can always write batch size bytes. This argument is updated on return with the actual configured value.
 *			In the listener case, this means the application will __never__ be woken up by the stack if the number of bytes available never reaches batch size(as can happen for example at the end of a video stream). The application should implement a time-out mechanism and make appropriate ::genavb_stream_receive \if LINUX / ::genavb_stream_receive_iov \endif calls to retrieve pending data if needed.
 * \param flags		may have the following bits set:
 * * ::AVTP_NONBLOCK to have the send/receive functions return immediately with the currently available data, even if it less than requested. If this flag is not set, the function call will block until all requested data is received or transmitted.
 * 			Blocking mode hasn't been implemented yet, so the send/receive functions will always behave in non-blocking mode.
 */
int genavb_stream_create(struct genavb_handle *genavb, struct genavb_stream_handle **stream, struct genavb_stream_params const *params,
							unsigned int *batch_size, genavb_stream_create_flags_t flags);


/** Receive media data from a given avb stream.
 * \ingroup stream
 * \return amount copied (in bytes, or negative error code (e.g invalid handle for stream receive). May be less than requested by data_len in case:
 * * not enough data is available,
 * * event cannot hold all the timestamps,
 * * an EOF (End-Of-Frame) event was encountered,
 * * of packet loss (will stop at last packet before sequence number discontinuity),
 * * of media clock restart (mr bit set in AVTP header).
For the last 2 cases, the data will stop at the last packet before the "event", i.e. last packet before the discontinuity, or last packet before the one with the mr bit set. flags will also be set on the *next* batch to signal the event at the start of that new batch. For the first 2 cases, no flags will be set and the call will be considered successful (data and timestamps are copied into data_iov and event_iov, whichever fills up first).
For the EOF event, data will stop with the byte matching the EOF, so that the EOF will always be the last event returned.
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data			buffer where stream data is to be copied.
 * \param data_len		length of the data in bytes.
 * \param event			array where events matching the datas are to be copied (see genavb_event).
 * \param event_len		pointer to the length of the event array (in struct genavb_event units). On return, will contain the number of events actually present. If event_len is NULL, only data will be returned.
 */
int genavb_stream_receive(struct genavb_stream_handle const *stream, void *data, unsigned int data_len,
			struct genavb_event *event, unsigned int *event_len);


/** Send media data on a given AVTP stream.
 * \ingroup stream
 * \return 			amount copied (in bytes), or negative error code (e.g invalid fd for stream receive). May be less than requested in case:
 * * not enough network buffers were available.
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data			buffer containing the data to send.
 * \param data_len		length of the data in bytes.
 * \param event			event structure array timestamps/flags for the data to be sent (see genavb_event).
 * \param event_len		length of the event array (in struct genavb_event units)
 */
int genavb_stream_send(struct genavb_stream_handle const *stream, void const *data, unsigned int data_len,
			struct genavb_event const *event, unsigned int event_len);


/** Destroy a given AVTP stream.
 * \ingroup stream
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param stream	stream handle for the stream to be destroyed.
 */
int genavb_stream_destroy(struct genavb_stream_handle *stream);


/** Return the presentation offset for a given stream
 * \ingroup stream
 * \return		Presentation offset (in ns) defining the lower limit of the ::AVTP_SYNC timestamps the stack will accept (for a talker stream), such that:
 * 				\<AVTP_SYNC timestamp\> >= \<current PTP time\> + \<presentation offset\>
 * \param handle	Handle of the stream to get the presentation offset for.
 */
unsigned int genavb_stream_presentation_offset(const struct genavb_stream_handle *handle);

/* OS specific headers */
#include "os/streaming.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_STREAMING_API_H_ */
