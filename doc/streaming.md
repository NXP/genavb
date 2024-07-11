Streaming API usage {#streaming_usage}
======================================

The streaming API makes it possible to create, destroy AVTP streams, and to send and receive data
to/from those streams.

Although the AVTP protocol is inherently packet-oriented, the GenAVB stack
hides the encapsulation/decapsulation work from the application, and instead exposes a
stream-oriented API: the data is viewed as a continuous stream of bytes, in order to relieve the
application from most of the low-level protocol details. Additional information, if present (lost
data, end of frames for video streams, etc), is conveyed "out-of-band", as an array of ::genavb_event
provided alongside the data bytestream.

Streams are created using the ::genavb_stream_create function (see below). Following the AVTP logic,
a stream represents a unidirectional flow of data between a Talker (the entity sending data over
the AVB network) and a Listener (the entity receiving that data). When data is ready to be sent or
received (see below @ref flow_control), the
application may use ::genavb_stream_send, \if LINUX ::genavb_stream_send_iov, \endif ::genavb_stream_receive \if LINUX,
::genavb_stream_receive_iov \endif functions to send or receive data, depending on the stream direction. When
a stream is no longer needed, it may be destroyed with the ::genavb_stream_destroy function.



# Stream creation {#stream_creation}
Creation of an AVTP stream on an endpoint can be done using the ::genavb_stream_create function. This
function must be provided with an ::genavb_stream_params structure, which may be initialized in two
different ways:
* Static mode
* AVDECC mode

### Static mode
The application must set all fields of ::genavb_stream_params by itself. Some helpers macros are
provided in the genavb header files to ease that process, in particular for the
::genavb_stream_params.format.

### AVDECC mode
The :genavb_stream_params structure is provided to the application by the AVDECC component of the
GenAVB stack, using an ::GENAVB_MSG_MEDIA_STACK_CONNECT message sent to the application on a
::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. The application can receive the message (which maps to the
::genavb_stream_params structure) using the ::genavb_control_receive call. See @ref control_usage for
details on how to receive such messages. Macros are also available to help extract information
from the ::genavb_stream_params.format field.
When relying on AVDECC, an ::GENAVB_MSG_MEDIA_STACK_CONNECT message is triggered in several

situations:
* Fast-connect mode: this mode is described in section 8.2.2.1.1 of the IEEE 1722.1-2013 standard.
A listener sends an ::ACMP_CONNECT_TX_COMMAND to a talker once it discovers a talker that
matches the saved state on the listener. On the Linux platform, that saved state (entity id,
stream unique id) can be provided to the GenAVB stack through run-time options.
* Back-to-Back (BTB) mode: this is a non-standard variation of Fast-connect, where the listener
tries to connect to the first talker discovered on the network. This mode is used in several
GenAVB demos where it is known in advance the setup has only one talker advertising itself
on the network.
* Controller-connect mode: this mode is described in section 8.2.2.1.3 of the IEEE 1722.1-2013
spec. A controller initiates the stream connection by sending a ::ACMP_CONNECT_RX_COMMAND to a
listener, and receives a ::ACMP_CONNECT_RX_RESPONSE from that listener once the stream has
been successfully connected.

Once a listener decides to connect a stream (based on one of the previous situations) the sequence
of events remains the same in all cases, and is described by the solid black arrows in the
following diagram.

![ACMP controller-connect and fast-connect modes](acmp_connect.png)
Diagram legend:
* Gray events are only present in controller-connect mode.
* Black events are present in all modes (fast-connect, BTB, controller-connect).
* single-line arrows: network packets.
* dual-line arrows: messages/function calls between customer application and GenAVB stack.


# Flow control {#flow_control}

\if LINUX

Using the file descriptor returned by the ::genavb_stream_fd function, an application may call
poll/epoll/select system calls to sleep and be woken up only when received data is available or
when buffer space is available for data to transmit. The amount of data that triggers a
wake-up is configured through the batch_size argument of ::genavb_stream_create. The GenAVB stack
only processes data one AVTP packet at a time, so the configured batch size is always a
multiple of the payload size of an AVTP packet for the given stream.
With VBR streams (such as compressed video formats like 61883-4, CVF/MJPEG), using a timeout with
poll/epoll/select system calls is strongly recommended, to ensure that no stale data remains in
the stack because the batch size hasn't been reached yet (see below for a more detailed
discussion).

\else

A callback can be registered using ::genavb_stream_set_callback which is called when received data is available or when buffer space is available for transmit. The callback must not block and should notify another task which performs the actual data reception/transmission. After the callback has been called it must be re-armed by calling ::genavb_stream_enable_callback. This should be done after the application has read all data available/written all data available (and never from the callback itself).
The amount of data that triggers a wake-up is configured through the batch_size argument of ::genavb_stream_create. The GenAVB stack only processes data one AVTP packet at a time, so the configured batch size is always a multiple of the payload size of an AVTP packet for the given stream.

\endif

# Receiving data (listener stream) {#rx_data}
Reception of stream data can be done through ::genavb_stream_receive \if LINUX and ::genavb_stream_receive_iov.  
These functions behave the same, except for the way the data buffers are passed along:
\else
  

\endif
* ::genavb_stream_receive accepts a single pointer to the memory area where data should be copied,
along with the buffer length,

\if LINUX
* ::genavb_stream_receive_iov uses an array of ::genavb_iovec, making it possible to use a
scatter-gather scheme and have data be copied directly into various memory buffers.

Both functions are non-blocking, so they may return less data than requested if not enough is
available.

\else

This function is non-blocking, so it may return less data than requested if not enough is
available.

\endif

If the event* arguments are set, events that may have occurred in the stream are returned as
well. Common events may be timestamping information, end-of-frames, but also non-recurring
events/errors, such as packet loss. To ease the processing on the application side, some events
force "short reads", where the stack returns less data than requested even if more was
available. When that happens, reading the last returned event provides an easy way for the
application to determine what happened and take appropriate action.

Different stream formats behave in different ways, so some types of events may or may not be
relevant in all cases. The following sections list the various events available for each
stream format and their meaning.

### Audio
#### 61883-6
TBD

#### AAF
TBD

### Video
#### 61883-4
* AVTP timestamps are normally present at the beginning of every MPEG Transport Packet (188
bytes), unless the ::AVTP_TIMESTAMP_INVALID flag is set in the event mask.
* ::AVTP_PACKET_LOST is set when data was lost. The GenAVB stack considers data was lost
when the AVTP sequence numbers of successive packets are not contiguous. This event forces a short
read.

Because the GenAVB stack only wakes up the application after at least batch_size bytes are
available, an application that doesn't use a timeout with poll/epoll/select system calls may
receive some data after its presentation time has expired, because the data remained in the GenAVB
stack as part of a partial batch. This may for example happen:
* when reaching the end of a video,
* with sparse streams consisting of long periods of silence between bursts of data
It is therefore recommended to always set a timeout with poll/epoll/select, and when a timeout
occurs, to read the pending data (whose amount will be less than batch_size). The timeout value
can be determined based on the additional buffering that is done within the application to process
the data: for example, with a Gstreamer pipeline that adds several hundreds of ms latency, a value
of 100ms would be a reasonable value to make sure data is received in time without starving the
Gstreamer pipeline, while also preventing the timeout from happening all the time (which would
negate the advantages of using poll/epoll/select).


#### CVF/MJPEG
* ::AVTP_PACKET_LOST is set when data was lost. This event forces a short read. The GenAVB stack
considers data was lost in several situations:
  * when the AVTP sequence numbers of successive packets are not contiguous,
  * when the fragment_offset fields of the MJPEG headers of successive packets show a discontinuity
  or an overlap.
* ::AVTP_END_OF_FRAME is set on the last byte of a video frame. This is determined by the
GenAVB stack based on the M bit of the AVTP header. That event also forces a short read, to make
it easier for the application to group together data for a single NALU.


#### CVF/H264
* ::AVTP_PACKET_LOST is set when data was lost. This event forces a short read. The GenAVB stack
considers data was lost when the AVTP sequence numbers of successive packets are not contiguous.
* ::AVTP_END_OF_FRAME is set on the last byte of a NALU. This is determined by the
GenAVB stack based on:
  * The last FU-A packet.
  * A single NALU packet
  * The Marker bit.
  * The last NALU of a STAP packet. If a STAP packet contains several NALUs, only the last one will have an event. The application needs to do custom h264 parsing to get NALUs seperated.
That event also forces a short read, to make it easier for the application to group together data for a single frame.
For H264,  the timestamps passed to the application layer correspond to H264 timestamps as defined in IEEE1722-2016 Section 8.5.3.1 (and not avtp timestamps)
Thus, ::AVTP_TIMESTAMP_INVALID is used to indicate the validity of the h264 timestamp.

### Control

#### ACF/NTSCF
The following events can be propagated to the media interface:
* ::AVTP_PACKET_LOST is set when data was lost.

#### ACF/TSCF
The following events can be propagated to the media interface:
* ::AVTP_TIMESTAMP_INVALID/::AVTP_TIMESTAMP_UNCERTAIN are set to indicate the validity of the avtp timestamp
* ::AVTP_PACKET_LOST is set when data was lost

> Note: if none of the ::AVTP_TIMESTAMP_INVALID/::AVTP_TIMESTAMP_UNCERTAIN/::AVTP_PACKET_LOST events are set,
a valid timestamp can be read from the events array.

# Sending data (talker stream) {#tx_data}
Sending stream data can be done through ::genavb_stream_send \if LINUX and ::genavb_stream_send_iov.  
These functions behave the same as their receive counterparts, except they are used for sending
\else
   

This function behave the same as its receive counterparts, except it's used for sending
\endif
data instead of receiving data.

Compared to the receive direction, another set of events is used to reflect the different
requirements of the talker side.


### Audio
#### 61883-6
TBD

#### AAF
TBD

### Video
#### 61883-4
AVB stream reservations (using SRP) usually result in fairly small packets being sent at a
fairly high rate. For example, a 24Mbps Class A SRP reservation for a 61883-4 stream would be done
by using a max AVTP payload size of 384 bytes (2 MPEG TS packets, (188+4)*2), and a rate of 8kpps.
However, because a 61883-4 stream is usually VBR, the actual rate is most of the time lower.
As a consequence, the GenAVB stack needs to have timing information for each packet in order
to respect that variable rate (and to provide accurate timestamps inside the packets).

This packetization scheme results in the following constraints:
* ::AVTP_SYNC events with valid timestamps should be added to mark the beginning of every MPEG
Transport Packet, to ensure the stack has the information it needs to send AVTP packets at an
accurate time and with valid AVTP timestamps.
* ::AVTP_FLUSH events should be added as the first event when calling
::genavb_stream_send \if LINUX / ::genavb_stream_send_iov\endif, if the associated data should be sent immediately even if
it results in a partial packet being sent. This is useful for example:
  * when reaching the end of a video, to ensure data for the last frame is sent without delay,
  * or with sparse streams when the amount of data is often small and the bitrate very
  irregular (such as when sending display updates for a remote GUI)

> Note: because of current GenAVB API limitations, an application cannot set the rate to use with
> a given stream, and as a result all 61883-4 streams have a hard-coded 24 Mbps bitrate. Since it
> is only a maximum, a lot of use cases are expected to be covered by that value, but customers with
> more specific needs are invited to get in touch with the GenAVB team to discuss possible
> solutions. This constraint will likely be removed in a future release.

\if LINUX
#### CVF/H264

the H264 stream has different packetization modes (Single Nal Unit, Fragmentation unit FU, STAP and MTAP)
defined in RFC6184.
The first constraint is that the header size for different modes is not the same and the NALU
sizes is not fixed. That's why a parsing process is needed to prepare the packets with proper header regions
before sending them to the stack.

A special function is dedicated for that : ::genavb_stream_h264_send
Constraints for this function are:
- Data buffer should only contain a partial NALU (the beginning, a middle section or just the end) but the order
  of the byte stream must be respected.
- The start of an NALU must always be at the start of the data buffer.
- The last bytes of the NALU should be sent with an ::AVTP_FRAME_END event

The AVB stream reservation will specify the maximum single nal unit size (equal to max AVTP payload size).
Thus, NAL units bigger than the max AVTP payload size are sent as fragmentation units, otherwise it will be
sent as a single packet.
The event.ts field is used to pass, presentation timestamps that will be set to the h264_timestamp field
of the AVTP H264 packet.

The following events should be used for described use cases:

* ::AVTP_SYNC events with valid presentation timestamps should be added with the beginning of every NALU
  to be passed later as h264_timestamp

* ::AVTP_FRAME_END event should be added to mark the end of a NALU. The associated data should
  be sent immediately.

> Note: because of current GenAVB API limitations, an application cannot set the rate to use with
> a given stream, and as a result all H264 streams have a hard-coded 24 Mbps bitrate. Since it
is only a maximum, a lot of use cases are expected to be covered by that value.

\endif

### Control

AVTP Control streams are relying on a Datagram mode by creating the stream using the ::AVTP_DGRAM flag. In this mode
the framing task is under the application responsability, with the AVB stack simply adding/removing
AVTP and Ethernet headers to packets coming from/going to the application.
The flag ::GENAVB_STREAM_FLAGS_CUSTOM_TSPEC shall be set in the stream's parameters flag meaning that the application
defines the stream's properties which are defined by IEEE-1722 stream traffic specification
(TSpec): payload size of the packets and number of packets per class interval.

#### ACF/NTSCF
Asynchronous format which does not require any presentation timestamp event to be filled. Any
timestamp event received by the stack for this mode is silently ignored.

#### ACF/TSCF
Synchronous format requiring a presentation timestamp event for each packet.
Any call to the stream send API without a timestamp will be treated as an error.

