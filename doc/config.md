GenAVB/TSN Configuration {#genavb_config}
==================================

The GenAVB/TSN stack is configured at startup through different configuration files.


---


# Main Configuration

The main configuration file for GenAVB/TSN stack is under: `/etc/genavb/config`

The general parameters defined in the `/etc/genavb/config` are the following:

### Auto start
Parameter		| Format & Range
 ----------------------	| :-----------
CFG_AUTO_START		| 0 or 1 (default 1)

Set to 1 to start the AVB stack automatically at system startup time. Set to 0 will require starting the AVB stack manually (# avb.sh start).

> Note: This option is only functional on systems with sysvinit as startup program. For target with systemd, a genavb-tsn.service is available to handle automatic startup.

### Setting system clock to be gPTP-based
Parameter		| Format & Range
 ----------------------	| :-----------
CFG_USE_PHC2SYS		| 0 or 1 (default 1)

Set this parameter to 1 in order to base the system clock on gPTP time. This will ensure that applications (like gstreamer) using the system clock have their process gPTP-based.

### GenAVB/TSN endpoint configuration mode

On targets that support both Endpoint TSN and Endpoint AVB, the configuration mode is set by the `GENAVB_TSN_CONFIG`  parameter (depending on available configs):

> GENAVB_TSN_CONFIG=1  
GENAVB_TSN_CONFIG 1 - Endpoint TSN  
GENAVB_TSN_CONFIG 2 - Endpoint AVB


---


# GenAVB/TSN Endpoint Configuration profile

The `GENAVB_TSN_CFG_FILE` previously set in `/etc/genavb/config` (depending on the configuration mode: Endpoint AVB or Endpoint TSN) is made of a pair of configuration files.

Parameter              | Format & Range | Description
 ----------------------| :----------- | :-----------
APPS_CFG_FILE          | Text string (default `/etc/genavb/apps-listener-alsa.cfg` for Endpoint AVB and `/etc/genavb/apps-tsn-network-controller.cfg` for Endpoint TSN) | Points to a apps-*.cfg file containing a demo configuration (media apps to use, controller option...). It is parsed by the startup script avb.sh
GENAVB_CFG_FILE        | Text string (default `/etc/genavb/genavb-listener.cfg`) | **Only valid for Endpoint AVB** : Points to a genavb-*.cfg file containing the configuration of the AVB stack, and is parsed by the avb application.

There are several pre-defined profiles, which can be used to set the role and behavior of the Endpoint node. Set these parameters according to the description of each demo preparation steps.

---


# Application profile parameters
The profile parameters are defined in the apps-*.cfg files of the `/etc/genavb/` directory. Several profiles are already defined, targeted to suit example usage scenarii. The meaning of the parameters is as follows:

### Defining a media application
Parameter			| Format & Range
 ------------------------------	| :-----------
CFG_EXTERNAL_MEDIA_APP		| Text string (default alsa-audio-app for Endpoint AVB and tsn-app for Endpoint TSN)
CFG_EXTERNAL_MEDIA_APP_OPT	| Text string in double quotes (default "")

These parameters indicate the name of the media application binary to be loaded along with the GenAVB/TSN stack, and its associated input parameters. 
The media application binary must be present in the file system of the target under the /usr/bin repository. The application name must be specified as absolute path or be in the default PATH.

### Defining a control application (Valid only for Endpoint AVB)
Parameter			| Format & Range
 ------------------------------	| :-----------
CFG_USE_EXTERNAL_CONTROLS	| 0 or 1 (default 0)
CFG_EXTERNAL_CONTROLS_APP	| Text string (default genavb-controls-app)

These parameters enable and indicate the name of the control application binary to be loaded along with the AVB stack. Set this parameter to 1 and specify the control application name to enable such application type to handle AECP controls commands such as volume control from the other entity.
The control application binary must be present in the file system of the target under the /usr/bin repository. The application name must be specified as absolute path or be in the default PATH.


---


# Endpoint AVB stack profile parameters

The Endpoint AVB stack parameters are defined in the genavb-*.cfg files of the `/etc/genavb/` directory. Several profiles are already defined, targeted to suit example usage scenarii.
Cfg files are organized by sections (noted [XXX]), containing several key/value pairs.

Use # to comment lines.
When a section or key/value pair is missing in the cfg file, default values will be used.
The meaning of the section and keys is as follows:

### Section [AVB_GENERAL]
Key			| Value & Range	| Description
 ----------------	| :-----------	| :-----------
log_level		| Text string. Can be 'crit', 'err', 'init', 'info', 'dbg'. (default: info) | Sets log level for all stack components
disable_component_log	| Text string: 'avtp', 'avdecc', 'srp', 'gptp', 'common', 'os', 'api', or 'none'. (default : none) | Disable log for one or more components. Use a comma separated list to specify several components, eg: disable_component_log = avtp, avdecc
log_monotonic	| Text string: 'disabled' or 'enabled'. (default: disabled)	| Controls if monotonic timestamps are included in the logs output

### Section [AVB_AVDECC]
Key		| Value & Range | Description
 ---------------| :-----------	| :-----------
enabled		| 0 or 1 (default 1) | Enables AVDECC stack component. If disabled, all other [AVB_AVDECC_xxx] sections parameters are unusued and the media application is responsible for stream creation/destruction, as well as MSRP stream declaration. 0 - disabled, 1 - enabled.
milan_mode	| 0 or 1 (default 0) | Enables AVDECC stack to run according to AVnu MILAN specifications, otherwise it works according to IEEE 1722.1 specification. 0 - IEEE 1722.1 Mode, 1- AVnu Milan mode.
association_id	| Unsigned, 64 bits (default 0)  | Association ID to advertise for the local entities (see custom AEM parameters below).
max_entities_discovery	| Unsigned (min 8, max 128, default 16)  | Maximum number of discoverable AVDECC entities


**Below options are only available for milan_mode = 0**

Key		| Value & Range | Description
 ---------------| :-----------	| :-----------
srp_enabled	| 0 or 1 (default 1) | Enables SRP API's to be called directly from AVDECC stack component (and stream reservations to be declared/redrawn by AVDECC/ACMP as streams are connected/disconnected). If disabled the media application is responsible for handling stream reservations. 0 - disabled, 1 - enabled.
fast_connect	| 0 or 1 (default 0) | Enables fast connect mode (Listener entity will connect streams automatically, based on saved talker information). 0 - disabled, 1 - enabled.
btb_demo_mode	| 0 or 1 (default 0) | Back to back demo mode: 0 - disabled, 1 - enabled, default: disabled. When enabled this entity will connect to the first talker advertised on the network. This is a demo mode (the other entity shouldn't have this mode enabled). This will force fast_connect to 1. 0 - disabled, 1 - enabled.

### Section [AVB_AVDECC_ENTITY_1]
Key			| Value & Range	| Description
 ----------------	| :-----------	| :-----------
entity_id		| \<id\>. \<id\> is unsigned, 64bits (default: 0) | ID of the entity (see custom AEM parameters below). This is a EUI-64 identifier unique to the entity.
entity_file		| Text string. Format: `/etc/genavb/\<file_name\>.aem` | File to be used as AVDECC AEM entity description. There are several pre-defined descriptions, which can be used to set the capabilities of the AVB node. Default : none. See chapter 4 below for more details about AEM entities.
channel_waitmask	| bitmask, between 0 and 7 included. default: 0	| The stack waits for the specified control channels to be opened by the application before starting AVDECC for the entity: bit 0 => MEDIA_STACK channel, bit 1 => CONTROLLER channel, bit 2 => CONTROLLED channel.
max_listener_streams	| Unsigned (min 1, max 64, default 8)  | Maximum number of listener streams supported for this AVDECC entity.
max_talker_streams	| Unsigned (min 1, max 64, default 8)  | Maximum number of talker streams supported for this AVDECC entity.
max_inflights		| Unsigned (min 5, max 128, default 5) | Maximum number of simultaneous inflight commands for this AVDECC entity.
max_unsolicited_registratons	| Unsigned (min 1, max 64, default 8) | Maximum number of unsolicited notifications registration for this AVDECC entity.
max_ptlv_entries	| Unsigned (min 1, max 179, default 16)  | Maximum number of tracked clock ids in the path trace by AVDECC


**Below options are only available for milan_mode = 0**

Key			| Value & Range	| Description
 ----------------	| :-----------	| :-----------
talker_entity_id_list	| \<id\>[,\<id\>,\<id\>]. \<id\> is unsigned, 64 bits (default : none) | Talker entity ID list. Used only in fast-connect mode.
talker_unique_id_list	| \<id\>[,\<id\>,\<id\>]. \<id\> is unsigned, 16 bits (default : none) | Talker unique ID list. Used only in fast-connect mode.
listener_unique_id_list	| \<id\>[,\<id\>,\<id\>]. \<id\> is unsigned, 16 bits (default : none) | Listener unique ID list. Used only in fast-connect mode.
valid_time = 60		| Unsigned (min 2, max 62, default 62)	| The valid time period for the AVDECC entity in seconds.
max_listener_pairs	| Unsigned (min 1, max 512, default 10)	| Maximum number of connected listeners per talker for this AVDECC entity.

### Section [AVB_AVDECC_ENTITY_2]
It is possible to define a second AEM entity on the same node. If used, the second entity can only be an AVDECC controller.
Key			| Value & Range	| Description
 ---------------	| :-----------	| :-----------
entity_file		| Text string. Format: `/etc/genavb/controller.aem` (default : none) | File to be used as AVDECC AEM entity description.
channel_waitmask	| bitmask, between 0 and 7 included. default: 0	| The stack waits for the specified control channels to be opened by the application before starting AVDECC for the entity: bit 0 => MEDIA_STACK channel, bit 1 => CONTROLLER channel, bit 2 => CONTROLLED channel.


### Custom AEM parameters
Except for a few specific items, all the fields of an AEM entity take their value from the AEM
entity description file (see section 4 below).The parameters that behave differently are:
* the association ID
* the entity ID
* the entity model ID

#### Association ID
If the association_id parameter of the [AVB_AVDECC] section of the configuration file is set to a
non-zero value, it will override any value set in the AEM definition files, for all entities on
the endpoint. If it is unset or 0, the value stored in each entity AEM definition file will be used
instead.

> For each entity, a non-zero association_id value remains dependent on the
> ADP_ENTITY_ASSOCIATION_ID_SUPPORTED flag being set in the entity_capabilities field of the AEM
> definition file. If that flag isn't set, the association_id will be forced to 0 at init time. The
> ADP_ENTITY_ASSOCIATION_ID_VALID flag will also be set or cleared at init time, based on the value
> of the assocation_id field (independently of the initial value stored in the AEM definition file).

#### Entity ID
The entity ID will be set according to the following, in decreasing order of priority:
* the entity_id parameter of the [AVB_AVDECC_ENTITY_n] section of the configuration file,
if it is set and non-zero,
* the entity ID value stored in the AEM definition file, if it is non-zero,
* a dynamic value computed from the Freescale OUI-24 (00:04:9f), the last 3 bytes of the eth0 MAC
address, and the entity index on the endpoint (0 or 1).

(so the configuration file value, if present, will override any value which may have been set otherwise).

> To ensure uniqueness, the entity ID shall be set by the entity manufacturer using an EUI block that it owns.

#### Entity model ID
 The entity model ID will be set to:
 * the entity model ID stored in the AEM definition file, if it is non-zero,
 * the entity ID value otherwise.


---


# Defining an AVDECC entity description

The current descriptions are using the following:

Audio characteristics:
* AVTP Format IEC 61883-6 AM824 or AAF Format (when specified)
* 2 audio channels
* 48 KHz sample rate
* 24 bits

Video characteristics:
* AVTP Format IEC 61883-4
* MPEG2-TS compressed audio/video
* Maximum bit rate 24Mbps

The pre-defined descriptions files are as follows:

* Audio talker/listener, single stream
  + AVDECC Entity configuration file: listener_talker_audio_single.aem

* Audio Milan talker/listener, single stream
  + AVDECC Entity configuration file: listener_talker_audio_single_milan.aem

* Audio Milan listener, single stream
  + AVDECC Entity configuration file: listener_audio_single_milan.aem

* Audio Milan talker, single stream
  + AVDECC Entity configuration file: talker_audio_single_milan.aem

* Audio listener, single stream
  + AVDECC Entity configuration file: listener_audio_single.aem

* Audio talker, single stream
  + AVDECC Entity configuration file: talker_audio_single.aem

* Audio talker + listener, multiple streams (8 + 8 streams)
  + AVDECC Entity configuration file: talker_listener_audio_multi.aem

* Audio talker + listener, multiple streams, AAF format (8 + 8 streams)
  + AVDECC Entity configuration file: talker_listener_audio_multi_aaf.aem

* Audio/Video listener, single stream
  + AVDECC Entity configuration file: listener_video_single.aem

* Audio/Video talker, single stream
  + AVDECC Entity configuration file: talker_video_single.aem

* Raw Audio + MPEG2-TS talker, dual stream
  + AVDECC Entity configuration file: talker_audio_video.aem

* Controller
  + AVDECC Entity configuration file: controller.aem


### AEM configuration file format
The AVDECC entity description is stored in some AEM configuration files, used to set the capabilities of the AVB node. The binary format of the AEM file is as follows.
The file starts with a fixed header with information on the number of each descriptor type, present in the file, and their size. All descriptors of a given type have the same size. There are 38 types of descriptors as specified in IEEE Std 1722.1-2013, section 7.2.

offset (bytes) | width (bytes) | Descriptor type  | name            | endianess 
 --------------| :-----------: | :--------------: | :-------------: | :---------:
0              | 2             | 0                | size_0 (bytes)  | LE 
2              | 2             | 0                | total_0         | LE 
4              | 2             | 1                | size_1 (bytes)  | LE 
6              | 2             | 1                | total_1         | LE 
...            | -             | -                | -               |-
37 * 4         | 2             | 37               | size_37 (bytes) | LE 
37 * 4 + 2     | 2             | 37               | total_37        | LE 

This header is followed by the actual descriptors. All descriptors follow the format described in IEEE Std 1722.1-2013 section 7.2. If the total number of descriptors is 0, for a given descriptor type, the descriptor is skipped. For variable sized descriptors there may be more space used in the file than required in memory.

offset (bytes)                                      | width (bytes) | Descriptor type | Descriptor Number
 ---------------------------------------------------| :-----------: | :-------------: | :-----------------:
152                                                 | size_0        | 0               | 0
152 + size_0                                        | size_0        | 0               | 1
...                                                 | -             | -               | -
152 + (total_0 - 1) * size_0                        | size_0        | 0               | total_0 - 1
152 + total_0 * size_0                              | size_1        | 1               | 0
152 + total_0 * size_0 + size_1                     | size_1        | 1               | 0
...                                                 | -             | -               | -
152 + total_0 * size_0 + (total_1 - 1) * size_1     | size_1        | 1               | total_1 - 1


---


# AVB Startup Options
The AVB stack accepts command line options which are documented in this section.

* -v				displays program version
* -f \<conffile\>			path and filename to read configuration from (eg: `/etc/genavb/genavb-listener.cfg`)
* -h				prints help text


---


# Endpoint, entity and application constraints
The AVB stack currently supports the following AVDECC roles:
* talker (both for streaming and control),
* listener(both for streaming and control),
* controller.

A single entity can combine several of these roles.
On a Linux endpoint, those roles may be handled by one or several applications using the AVB stack API, but channel communication constraints between the stack and the applications impose the following rules:
* genavb_control_open(.., .., AVB_CTRL_AVDECC_CONTROLLER) may only be called once (there can be only a single open instance in the entire system),
* genavb_control_open(.., .., AVB_CTRL_AVDECC_CONTROLLED) may only be called once (there can be only a single open instance in the entire system),
* genavb_control_open(.., .., AVB_CTRL_AVDECC_MEDIA_STACK) may only be called once (there can be only a single open instance in the entire system),
* a single control channel instance (opened with ::genavb_control_open) may only handle a single entity.

If we translate those rules in terms of entities, this means we are currently limited to at most 2 entities per endpoint:
* one being a streaming entity (talker, listener, or a single entity with both roles),
* one being a controller.
Having more than 2 is not possible, and having one talker and one listener or 2 talkers, or 2 listeners, or 2 controllers isn't either.

Examples:
* one listener entity, 1 streaming application, 1 controlled application => OK
* one listener entity, one controller entity, 1 streaming application, 1 controller and controlled application => OK
* one talker/listener entity, 1 application handling listener and talker streams as well as the controlled role => OK
* one talker entity, 1 application handling streaming, 2 applications for control => INCORRECT
* one talker/listener entity, 1 application for talker streams, one for listener streams => INCORRECT
* one talker entity, one controller entity, 1 application for talker streams, 1 for talker control, 1 for controller => OK
* one talker entity, one listener entity, any application combination => INCORRECT
* one listener/talker entity, one controller entity, one application handling everything => OK
* one listener/talker/controller entity, one application handling everything => OK

> Note: For extra correctness, one "application" in this context should actually be understood as one instance of the AVB library, as created by genavb_init.
