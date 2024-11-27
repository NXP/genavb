/*
 * Copyright 2018, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public API
 \details OS specific API definition for the GenAVB/TSN library

 \copyright Copyright 2018, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_STREAMING_API_H_
#define _OS_GENAVB_PUBLIC_STREAMING_API_H_

/** Retrieve the file descriptor associated with a given AVTP stream.
 * \ingroup stream
 * \return		socket/file descriptor (to be used with poll/select) for the stream, or negative error code.
 * \param stream	stream handle returned by genavb_stream_create.
 */
int genavb_stream_fd(struct genavb_stream_handle const *stream);


/** Send H264 media data on a given CVF H264 AVTP stream.
 *  This is a special function to transmit H264 NALU buffers on an AVTP stream.
 *  It will pad the data buffer to take into account the different H264 packetization headers
 *  and avoid un-necessary memory copies.
 *  The data buffer can contain at most one NALU and the start of an NALU must always be at
 *  the start of the data buffer.
 *  It's valid for the data buffer to only contain a partial NALU (the beginning, a middle
 *  section or just the end) but the order of the byte stream must be respected.
 *  The last bytes of the NALU should be sent with an ::AVTP_FRAME_END flag.
 *  The amount of data written at the start of a NALU must be sufficient to let the function decide the h264 packetization mode to use. If that's not the case -::GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA will be returned.
 * \ingroup stream
 * \return 			amount copied (in bytes), or negative error code.
 * * The amount copied may be less than requested in case not enough network buffers were available.
 * * The negative error code can be:
 * * * -::GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA: This means that the data (the start of NALU sent) is less than what the function needs to decide on packetization mode. The caller needs to resend on next media stack wakeup either an amount of data equal to batch size or the full NALU (with ::AVTP_FRAME_END flag).
 * * * Any other negative error means a fatal error is encountred (e.g sending new NALU without ::AVTP_FRAME_END flag for the previous NALU).
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data			buffer containing the data to send.
 * \param data_len		length of the data in bytes.
 * \param event			event structure array timestamps/flags for the data to be sent (see genavb_event).
 * \param event_len		length of the event array (in struct genavb_event units)
 */
int genavb_stream_h264_send(struct genavb_stream_handle *stream, void *data, unsigned int data_len,
			struct genavb_event *event, unsigned int event_len);


/** Receive media data from a given avb stream.
 * \ingroup stream
 * \return amount copied (in bytes, or negative error code (e.g invalid handle for stream receive). May be less than requested by data_iov in case:
 * * not enough data is available,
 * * event_iov cannot hold all the timestamps,
 * * an EOF (End-Of-Frame) event was encountered,
 * * of packet loss (will stop at last packet before sequence number discontinuity),
 * * of media clock restart (mr bit set in AVTP header).
For the last 2 cases, the data will stop at the last packet before the "event", i.e. last packet before the discontinuity, or last packet before the one with the mr bit set. flags will also be set on the *next* batch to signal the event at the start of that new batch. For the first 2 cases, no flags will be set and the call will be considered successful (data and timestamps are copied into data_iov and event_iov, whichever fills up first).
For the EOF event, data will stop with the byte matching the EOF, so that the EOF will always be the last event returned.
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data_iov		iovec array where stream data is to be copied.
 * \param data_iov_len		length of the data_iov array.
 * \param event_iov		iovec array where events are to be copied (see genavb_event). For each of the genavb_iovec's, the iov_base should point to an array of struct genavb_event and iov_len must be in struct genavb_event units.
 * \param event_iov_len 	length of the event_iov array. If event_iov_len is equal to zero, only data will be returned.
 * \param event_len		On return, the event_len pointer will contain the number of events actually copied to the event iovecs. This function will return an error if event_len is NULL, but event_iov is not NULL or event_iov_len is not zero.
 */
int genavb_stream_receive_iov(struct genavb_stream_handle const *stream, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_iovec const *event_iov, unsigned int event_iov_len, unsigned int *event_len);


/** Send media data on a given AVTP stream.
 * \ingroup stream
 * \return 			amount copied (in bytes), or negative error code (e.g invalid fd for stream receive). May be less than requested in case:
 * * not enough network buffers were available
 * \param stream		stream handle returned by ::genavb_stream_create.
 * \param data_iov		iovec array containing the data to send.
 * \param data_iov_len		length of the data_iov array.
 * \param event			event structure array timestamps/flags for the data to be sent (see genavb_event).
 * \param event_len		length of the event array (in struct genavb_event units)
 */
int genavb_stream_send_iov(struct genavb_stream_handle const *stream, struct genavb_iovec const *data_iov, unsigned int data_iov_len, struct genavb_event const *event, unsigned int event_len);


#endif /* _OS_GENAVB_PUBLIC_STREAMING_API_H_ */
