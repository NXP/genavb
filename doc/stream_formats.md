Stream formats {#stream_formats}
====================================

The GenAVB/TSN stack supports several AVTP encapsulation formats for the transport
of Audio, Video and Control information.
At the API level, when creating streams, stream formats are specified according to
the AVDECC format defined in IEEE 1722-2016, Annex I.

# Supported stream formats {#str_formats_0}
Format				| Direction 		| AVTP Subtype	| Media type	| Media Format
:---------			|---------		|-------	| ------	| ----
IEC 61883-4			|Talker + Listener	| 0x0		| Audio + Video	| MPEG2-TS
IEC 61883-6 AM824 MBLA	 	|Talker + Listener	| 0x0		| Audio		| AM824 24bit PCM Audio
IEC 61883-6 FLOAT32		|Talker + Listener	| 0x0		| Audio		| 32bit Floating point PCM Audio
IEC 61883-6 INT32		|Talker + Listener	| 0x0		| Audio		| 32bit Fixed point PCM Audio
AAF PCM	FLOAT32			|Talker + Listener	| 0x2		| Audio		| 32bit Floating point PCM Audio
AAF PCM	INT32			|Talker + Listener	| 0x2		| Audio		| 32bit Integer PCM Audio
AAF PCM	INT24			|Talker + Listener	| 0x2		| Audio		| 24bit Integer PCM Audio
AAF PCM	INT16			|Talker + Listener	| 0x2		| Audio		| 16bit Integer PCM Audio
AAF AES3			|Talker + Listener	| 0x2		| Audio		| AES3 Audio
CVF MJPEG			|Listener		| 0x3		| Video		| MJPEG Video
CVF H264			|Talker + Listener	| 0x3		| Video		| H264 Video
CRF AUDIO SAMPLE		|Talker + Listener	| 0x4		| Audio Sample Clock	| 64 bit timestamps
ACF TSCF			|Talker + Listener	| 0x5		| Control	| ACF Control Message
ACF NTSCF			|Talker + Listener	| 0x82		| Control	| ACF Control Message

The following sections describe the full range of valid parameters for each of the
supported formats.

--
# IEC 61883-4 {#str_formats_1}

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_61883_IIDC, sf = @ref IEC_61883_SF_61883, fmt = @ref IEC_61883_CIP_FMT_4

Any valid MPEG2-TS stream (with one or more elementary streams) can be transmitted and received.

--
# IEC 61883-6 AM824 {#str_formats_2}

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_61883_IIDC, sf = @ref IEC_61883_SF_61883, fmt = @ref IEC_61883_CIP_FMT_6,
fdf_evt = @ref IEC_61883_6_FDF_EVT_AM824

All audio rates are supported:

fdf_sfc = @ref IEC_61883_6_FDF_SFC_32000, @ref IEC_61883_6_FDF_SFC_44100, @ref IEC_61883_6_FDF_SFC_48000, @ref IEC_61883_6_FDF_SFC_88200,
@ref IEC_61883_6_FDF_SFC_96000, @ref IEC_61883_6_FDF_SFC_192000

Up to 32 channels are supported:

dbs = [1, 32]

Only MBLA samples are supported:

label_mbla_cnt = \[1, 32\] (must be equal to dbs)

label_iec_60958_cnt = 0

label_smpte_cnt = 0

label_midi_cnt = 0

Only non-blocking mode supported (no empty packets transmitted):

b = 0

nb = 1

Packetization is synchronous to media clock (packets contain fixed number of samples, expect for last packet in stream):

sc = x (ignored)

ut = x (ignored)

--
# IEC 61883-6 FLOAT32 {#str_formats_3}

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_61883_IIDC, sf = @ref IEC_61883_SF_61883, fmt = @ref IEC_61883_CIP_FMT_6,
fdf_evt = @ref IEC_61883_6_FDF_EVT_FLOATING

All audio rates are supported:

fdf_sfc = @ref IEC_61883_6_FDF_SFC_32000, @ref IEC_61883_6_FDF_SFC_44100, @ref IEC_61883_6_FDF_SFC_48000, @ref IEC_61883_6_FDF_SFC_88200,
@ref IEC_61883_6_FDF_SFC_96000, @ref IEC_61883_6_FDF_SFC_192000

Up to 32 channels are supported:

dbs = [1, 32]

Only non-blocking mode supported (no empty packets transmitted):

b = 0

nb = 1

Packetization is synchronous to media clock (packets contain fixed number of samples, expect for last packet in stream):

sc = x (ignored)

ut = x (ignored)

--
# IEC 61883-6 INT32 {#str_formats_4}

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_61883_IIDC, sf = @ref IEC_61883_SF_61883, fmt = @ref IEC_61883_CIP_FMT_6,
fdf_evt = @ref IEC_61883_6_FDF_EVT_INT32

All audio rates are supported:

fdf_sfc = @ref IEC_61883_6_FDF_SFC_32000, @ref IEC_61883_6_FDF_SFC_44100, @ref IEC_61883_6_FDF_SFC_48000, @ref IEC_61883_6_FDF_SFC_88200,
@ref IEC_61883_6_FDF_SFC_96000, @ref IEC_61883_6_FDF_SFC_192000

Up to 32 channels are supported:

dbs = [1, 32]

Only non-blocking mode supported (no empty packets transmitted):

b = 0

nb = 1

Packetization is synchronous to media clock (packets contain fixed number of samples, expect for last packet in stream):

sc = x (ignored)

ut = x (ignored)

--
# AAF PCM {#str_formats_5}
AVTP audio format with a PCM payload

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_AAF, format = @ref AAF_FORMAT_FLOAT_32BIT, @ref AAF_FORMAT_INT_32BIT, @ref AAF_FORMAT_INT_24BIT, @ref AAF_FORMAT_INT_16BIT

ut = 0, 1

All audio rates are supported:

nsr = @ref AAF_NSR_8000, @ref AAF_NSR_16000, @ref AAF_NSR_32000, @ref AAF_NSR_44100,
@ref AAF_NSR_48000, @ref AAF_NSR_88200, @ref AAF_NSR_96000, @ref AAF_NSR_176400, @ref AAF_NSR_192000, @ref AAF_NSR_24000

All bit depths are supported:

bit_depth = 0 (for @ref AAF_FORMAT_FLOAT_32BIT format)

bit_depth = 1, .., 32 (for @ref AAF_FORMAT_INT_32BIT format)

bit_depth = 1, .., 24 (for @ref AAF_FORMAT_INT_24BIT format)

bit_depth = 1, .., 16 (for @ref AAF_FORMAT_INT_16BIT format)


Up to 32 channels supported:

channels_per_frame = [1, 32]

Up to 256 samples per frame:

samples_per_frame = [1, 256]

--
# AAF AES3 {#str_formats_6}
AVTP audio format with AES3 encoded audio

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_AAF, format = @ref AAF_FORMAT_AES3_32BIT

ut = 0, 1

Up to 256 frames per channel are supported:

frames_per_frame = [1, 256]

Up to 10 streams per frame are supported:

streams_per_frame = 1, ..., 10

All stream data types and frames types are supported:

aes3_dt_ref = x (ignored)

aes3_data_type = x (ignored)

--
# CVF MJPEG {#str_formats_7}

Compressed Video format with MJPEG encoded Video

Only Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_CVF, format = @ref CVF_FORMAT_RFC, format_subtype = @ref CVF_FORMAT_SUBTYPE_MJPEG

Both interlaced and progressive scan types are supported:

p = @ref CVF_MJPEG_P_INTERLACE, @ref CVF_MJPEG_P_PROGRESSIVE

All types are supported:

type = @ref CVF_MJPEG_TYPE_YUV422, @ref CVF_MJPEG_TYPE_YUV420

All valid widths/heights are supported:

width > 0

height > 0

--
# CVF H264 {#str_formats_8}

Compressed Video format with H264 encoded Video

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_CVF, format = @ref CVF_FORMAT_RFC, format_subtype = @ref CVF_FORMAT_SUBTYPE_H264

Custom format parameters:

For H264 Listener, a custom field (rsvd1) enables a proprietary format, always set it to 0.
Default value is 0 for IEEE1722-2016 format specification.

--
# CRF AUDIO SAMPLE {#str_formats_9}

Clock Reference format with audio sample clock events

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_CRF, type = @ref CRF_TYPE_AUDIO_SAMPLE

timestamp_interval = 1, ..., 600

timestamps_per_pdu = 1, ..., 8

pull = @ref CRF_PULL_1_1, @ref CRF_PULL_1000_1001, @ref CRF_PULL_1001_1000, @ref CRF_PULL_24_25, @ref CRF_PULL_25_24, @ref CRF_PULL_1_8

base_frequency = 1, ..., 536870911 Hz

The timestamp frequency (base_frequency x pull / timestamp_interval) must respect:

300 Hz < frequency < 48000 Hz

--
# ACF TSCF {#str_formats_11}

AVTP Control Format support for Time Synchronous Control Format

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_TSCF

m = 0

type_0,1,2,3 = @ref ACF_MSG_TYPE_FLEXRAY, @ref ACF_MSG_TYPE_CAN, @ref ACF_MSG_TYPE_CAN_BRIEF, @ref ACF_MSG_TYPE_LIN, @ref ACF_MSG_TYPE_MOST, @ref ACF_MSG_TYPE_GPC
@ref ACF_MSG_TYPE_SERIAL, @ref ACF_MSG_TYPE_PARALLEL, @ref ACF_MSG_TYPE_SENSOR, @ref ACF_MSG_TYPE_SENSOR_BRIEF, @ref ACF_MSG_TYPE_AECP, @ref ACF_MSG_TYPE_ANCILLARY, @ref ACF_MSG_TYPE_USER

t0v = 0, 1

t1v = 0, 1

t2v = 0, 1

t3v = 0, 1

--
# ACF NTSCF {#str_formats_12}

AVTP Control Format support for Non Time Synchronous Control Format

Both Talker and Listener endpoints are supported:

direction = @ref AVTP_DIRECTION_LISTENER, @ref AVTP_DIRECTION_TALKER

Base format parameters:

v = @ref AVTP_VERSION_0, subtype = @ref AVTP_SUBTYPE_NTSCF

