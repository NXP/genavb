gPTP and SRP usage and configuration {#gptp_usage}
====================================

# Description

The GenAVB/TSN stack includes support for a gPTP component which implements the Generalized Precision Time Protocol as per IEEE 802.1AS-2020 specification
(a profile of PTPv2 IEEE 1588 standard) in order to establish an accurate time reference among all network nodes.

The gPTP component supports two different configurations:
 * Endpoint package (AED-E) : gPTP endpoint (tsn) capabilities (Slave or Grand Master)
 * Bridge package (AED-B): gPTP bridge (tsn -b) capabilities (Grand Master or Transparent clock)

The following features are supported:
 * Slave or GrandMaster capabilities
 * Best master clock selection algorithm (BMCA) for dynamic selection of the highest quality clock (GrandMaster)
 * Support for Automotive profiles per AVnu-AutoCDSFunctionalSpec v1.4 (static configuration, no BMCA, static pdelay, dynamic intervals)
 * Multiple gPTP Domains and Common Link Delay Service (CMLDS)

Also, the stack includes support for an SRP component which implements the Stream Reservation Protocol (SRP) as per IEEE Std 802.1Q-2018 Clause 35.
The SRP component supports two different configurations:
 * Endpoint package (AED-E) : SRP endpoint (tsn) capabilities
 * Bridge package (AED-B): SRP bridge (tsn -b) capabilities

On Linux, the gPTP and SRP stacks are part of the tsn process including also the Management component:

# Usage

    tsn [options]
    Options:
        -b starts bridge component
        -v display program version
        -f <gptp conf file> path and filename to read gPTP configuration from (eg: /etc/genavb/fgptp.cfg for endpoint stack or /etc/genavb/fgptp-br.cfg for bridge stack)
        -s <srp conf file> path and filename to read SRP configuration from (eg: /etc/genavb/srp.cfg for endpoint stack or /etc/genavb/srp-br.cfg for bridge stack)
        -h print this help

    Notes:
    The gPTP stack component will be launched once the first time the AVB stack is started.
    Depending on the node configuration (endpoint/bridge) one or more tsn processes are automatically started during system startup process.
    The default gptp configuration file (e.g.: fgptp.cfg) is for general gPTP parameters as well as domain 0 parameters. To enable other domains, new files must be created with '-N' appended to the filename (e.g.: 'fgptp.cfg-1' for domain 1).
    The tsn applications can also be stopped and started manually at any time using the following set of commands:
        tsn.sh stop
        tsn.sh start


# SRP Configuration files: /etc/genavb/srp[-br].cfg

### Section [SRP_GENERAL]

Key			| Value & Range	| Description
 ----------------	| :-----------	| :-----------
log_level		| Text string. Can be 'crit', 'err', 'init', 'info', 'dbg'. (default: info) | Sets log level for srp stack components
sr_class_enabled	| Text string: Can be 'A', 'B', 'C', 'D' or 'E'. (default: A,B)	| Select enabled SR classes, must be 2 different classes separated by a comma (order has no impact as alphabetical order will be applied)

### Section [MSRP]
Key			| Value & Range	| Description
 ----------------	| :-----------	| :-----------
enabled 		| 0 or 1 (default 1) | Set this configuration to 0 to disable MSRP processing on all ports

# GPTP Configuration files: /etc/genavb/fgptp.cfg[-N]


### Section [FGPTP_GENERAL]

#### Profile
The gptp stack can operate in two different modes known as 'standard' or 'automotive' profiles.

When the 'standard' profile is selected, the gptp stack operates following the specifications described in IEEE 802.1AS.
When the 'automotive' profile is selected, the gptp stack operates following the specifications described in the AVnu
AutoCDSFunctionalSpec_1.4 which is a subset of the IEEE 802.1AS specifications optimized for automotive applications.

The automotive environment is unique in that it is a closed system. Every network device is known prior to
startup and devices do not enter or leave the network, except in the case of failures. Because of the closed nature
of the automotive network, it is possible to simplify and improve gPTP startup performance. Specifically,
functions like election of a grand master and calculations of wire delays are tasks that can be optimized for a
closed system.

Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
profile | "standard" or "automotive" (default "standard") | Set gptp main profile.  "standard" - IEEE 802.1AS specs, "automotive" - AVnu automotive profile

#### Grand master ID
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
gm_id | 64bits EUI format (default "0x0001f2fffe0025fe") | Set static grandmaster ID in host order (used by automotive profile, ignored in case of standard profile)

####  GPTP domain number assignment
gPTP domain number param is a per-domain parameter (defined in the default and other domains config files).
At least one domain shall be supported: domain 0, with its domain_number equal to 0.

(see IEEE 802.1AS-2020 - 8.1 gPTP domain)

Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
domain_number | -1 to 127 (default -1 for non-zero instances) | Disable (value -1) or assign a gPTP domain number to a domain instance.

#### Log output level
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
log_level | 0 or 1 (default 0) | Set this configuration to 1 to enable debug mode

#### Reverse sync feature control
The Reverse Sync feature (Avnu specification) should be used for test/evaluation purpose only.
Usually to measure the accuracy of the clock synchronization, the traditional approach is to use
a 1 Pulse Per Second (1PPS) physical output. While this is a good approach, there may be cases
where using a 1PPS output is not feasible. More flexible and fully relying on SW implementation
the Reverse Sync feature serves the same objective using the standard gPTP Sync/Follow-Up messages
to relay the timing information, from the Slave back to the GM.

Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
reverse_sync | 0 or 1 (default 0) | Set to 1 to enable reverse sync.

#### Reverse sync feature interval
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
reverse_sync_interval | 32 to 10000 (default 112) | Reverse sync transmit interval in ms units

#### Neighbor propagation delay threshold
neighborPropDelayThresh defines the propagation time threshold, above which a port is not considered
capable of participating in the IEEE 802.1AS protocol (see IEEE 802.1AS - 11.2.2 Determination of asCapable).
If a computed neighborPropDelay exceeds neighborPropDelayThresh, 
then asCapable is set to FALSE for the port. This setting does not apply to Automotive profile where a
link is always considered to be capable or running IEEE 802.1AS.

Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
neighborPropDelayThresh | 32 to 10000000 (default 800) | Neighbor propagation delay threshold expressed in ns


#### Statistics output interval
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------
statsInterval | 0 to 255 (default 10) | Statistics output interval expressed in seconds. Use 0 to disable statistics.


### Section [FGPTP_GM_PARAMS]
This section defines the native Grand Master capabilities of a time-aware system (see IEEE 802.1AS - 8.6.2 Time-aware system attributes).
Grandmaster parameters define per-domain values (defined in separate config files).

gmCapable defines if the time-aware system is capable of being a grandmaster. By default gmCapable is set to 1
as in standard profile operation the Grand Master is elected dynamically by the BMCA. In case of automotive
profile gmCapable must be set on each AED node to match the required network topology (i.e. within a given
gPTP domain only one node must have its gmCapable property set to 1).

priority1, priority2, clockClass, clockAccuracy and offsetScaledLogVariance are parameters used by the
Best Master Clock algorithm to determine which of the Grand Master capable node whithin the gPTP domain
has the highest priority/quality. Note that the lowest value for these parameters matches the highest 
priority/quality.

#### Grandmaster capable setting
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
gmCapable |  0 or 1 (default 1) | Set to 1 if the device has grandmaster capability. Ignored in automotive profile if the port is SLAVE.

#### Grandmaster priority1 value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
priority1 | 0 to 255 (default 248 for AED-E and 246 for AED-B) | Set the priority1 value of this clock 

#### Grandmaster priority2 value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
priority2 | 0 to 255 (default 248) | Set the priority2 value of this clock

#### Grandmaster clock class value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
clockClass | 0 to 255 (default 248) | Set the class value of this clock

#### Grandmaster clock accuracy value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
clockAccuracy | 0x0 to 0xff (default 0xfe) | Set the accuracy value of this clock

#### Grandmaster variance value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
offsetScaledLogVariance | 0x0 to 0xffff (default 17258) | Set the offset scaled log variance value of this clock


### Section [FGPTP_AUTOMOTIVE_PARAMS]
The static pdelay feature is used only if the gPTP stack operates in automotive profile configuration.

At init time the gPTP stack's configuration file is parsed and based on neighborPropDelay_mode the specified
initial_neighborPropDelay is applied to all ports and used for synchronization until a pdelay response from the peer is
received. This is done only if no previously stored pdelay is available from the nvram database specified by nvram_file.
As soon as a pdelay response from the peer is received the 'real' pdelay value is computed, and used for current synchronization.
An indication may then be sent via callback up to the OS-dependent layer. Upon new indication the Host may update its nvram database
and the stored value will be used at next restart for the corresponding port instead of the initial_neighborPropDelay. The granularity 
at which pdelay change indications are sent to the Host is defined by the neighborPropDelay_sensitivity parameter.

In the gPTP configuration file the neighborPropDelay_mode parameter is set to 'static' by default, meaning that
a predefined propagation delay is used as described above while pdelay requests are still sent to the network.

The 'silent' mode behaves the same way as the 'static' mode except that pdelay requests are never sent at all to the network.

Optionally the neighborPropDelay_mode parameter can be set to standard forcing the stack to operate propagation delay
measurements as specified in the 802.1AS specifications even if the automotive profile is selected.

(see AutoCDSFunctionalSpec-1_4 - 6.2.2 Persistent gPTP Values)

#### PDelay mode
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
neighborPropDelay_mode | 'static' 'silent' or 'standard' (default static) | Defines pdelay mechanism used

#### Static pdelay value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
initial_neighborPropDelay | 0 to 10000 (default 250)| Predefined pdelay value applied to all ports. Expressed in ns.

#### Static pdelay sensitivity
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
neighborPropDelay_sensitivity | 0 to 1000 (default 10) | Amount of ns between two pdelay measurements required to trigger a change indication. Expressed in ns.

#### Path of the nvram file
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
nvram_file | (default /etc/genavb/fgptp.nvram) | Path and nvram file name.


### Section [FGPTP_PORTn]
Per port settings where n represents the port index starting at n=1.

Pdelay requests and Sync messages sending intervals have a direct impact on the system synchronization performance.
To reduce synhronization time while optimizing overall system load, two levels of intervals are defined.
The first level called 'Initial', defines the messages intervals used until pdelay values have stabilized and synchronization is achieved.
The second level called 'Operational', defines the messages intervals used once the system is synchronized.

initialLogPdelayReqInterval and operLogPdelayReqInterval define the intervals between the sending of successive Pdelay_Req messages.
initialLogSyncInterval and operLogSyncInterval define the intervals between the sending of successive Sync messages.
initialLogAnnounceInterval defines the interval between the sending of successive Announce messages

(see AutoCDSFunctionalSpec-1_4 - 6.2.1 Static gPTP Values)

#### Port role
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
portRole         | 'slave', 'master', 'disabled' (default disabled) | Static port role (ref. 802.1AS-2011, section 14.6.3, Table 10-1), applies to "automotive" profile only.

#### PTP port enabled
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
ptpPortEnabled   | 0 or 1 (default 1) | Set to 1 if both time-synchronization and best master selection functions of the port should be used (ref. 802.1AS-2011, sections 14.6.4 and 10.2.4.12).

#### Rx timestamping compensation delay
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
rxDelayCompensation         | -1000000 to 1000000 (default 0) | Rx timestamp compensation substracted from receive timestamp.

#### Tx timestamping compensation delay
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
txDelayCompensation         | -1000000 to 1000000 (default 0) | Tx timestamp compensation added to transmit timestamp.

#### Initial pdelay request interval value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
initialLogPdelayReqInterval | 0 to 3 (default 0) | Set pdelay request initial interval between the sending of successive Pdelay_Req messages. Expressed in log2 unit (default 0 -> 1s).

#### Initial sync interval value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
initialLogSyncInterval | -5 to 0 (default -3) | Set sync transmit initial interval between the sending of successive Sync messages. Expressed in log2 unit (default -3 -> 125ms).

#### Initial announce interval value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
initialLogAnnounceInterval | 0 to 3 (default 0) | Set initial announce transmit interval between the sending of successive Announce messages. Expressed in log2 unit (default 0 -> 1s).

#### Operational pdelay request interval value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
operLogPddelayReqInterval | 0 to 3 (default 0) | Set pdelay request transmit interval used during normal operation state. Expressed in log2 unit (default 0 -> 1s).

#### Operational sync interval value
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
operLogSyncInterval | -5 to 0 (default -3) | Set sync transmit interval used during normal operation state. Expressed in log2 unit (default -3 -> 125ms).

#### Pdelay Mechanism
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
delayMechanism | 'P2P', 'COMMON_P2P', 'SPECIAL' (default 'P2P' for domain 0, 'COMMON_P2P' for domains > 0)| Set peer delay mechanism associated to this port.

#### Allowed Lost Responses
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
allowedLostResponses | 1 to 255 (default 3 if force_2011 = yes, 9 otherwise) | Set the number of Pdelay_Req messages without valid responses above which this port is considered to be not exchanging peer delay messages with its neighbor.

#### Allowed Faults
Key              | Value & Range | Description
 ----------------| :-----------: | :-----------:
allowedFaults | 1 to 255 (default 9) | Set the number of faults above which asCapableAcrossDomains is set to FALSE, i.e., the port is considered not capable of interoperating with its neighbor. The term faults refers to instances where the computed mean propagation delay exceeds the threshold and/or the computation of neighborRateRatio is invalid.
