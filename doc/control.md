Control API usage {#control_usage}
============================

# Introduction

The control API has been designed as a generic interface to enable the configuration and control of various features of the
GenAVB stack by an external application.

It is a bi-directional message-passing interface, allowing the user-space application and the GenAVB stack to exchange
(send, receive) control messages through different control channels. Six channel types are currently defined (see @ref genavb_control_id_t):
* @ref GENAVB_CTRL_AVDECC_MEDIA_STACK : this channel can be used by a talker or listener application to be notified about
stream connections/disconnections done through AVDECC (either by an external controller, or through the fast-connect mechanism).
* @ref GENAVB_CTRL_AVDECC_CONTROLLER : to be used by an AVDECC controller application.
* @ref GENAVB_CTRL_AVDECC_CONTROLLED : to be used by an application running on a talker or listener, for AECP communication
(such as volume control, media track selection, play/stop control...).
* @ref GENAVB_CTRL_MSRP : this channel can be used by a talker or listener application to establish stream reservations and be notified
about the status of existing reservations on the network. If AVDECC is not being used, this API becomes mandatory to be able to
transmit and receive AVTP streams in an AVB network.
* @ref GENAVB_CTRL_MVRP : this channel can be used by a talker or listener application to establish VLAN registrations.
* @ref GENAVB_CTRL_CLOCK_DOMAIN : this channel can be used by a talker or listener application to set the clock source of a clock
domain and receive clock domain status indications.
* @ref GENAVB_CTRL_GPTP : this channel can be used by a talker or listener application to retrieve grand master information from GPTP
stack component.


Messages are represented by raw memory buffers, with an additional [message type](@ref genavb_msg_type_t) parameter to define their
type, and a msg_len parameter to specify their length. The following message types are currently available:
* @ref GENAVB_MSG_MEDIA_STACK_CONNECT and @ref GENAVB_MSG_MEDIA_STACK_DISCONNECT
* @ref GENAVB_MSG_AECP
* @ref GENAVB_MSG_ACMP_COMMAND and @ref GENAVB_MSG_ACMP_RESPONSE
* @ref GENAVB_MSG_ADP
* @ref GENAVB_MSG_LISTENER_REGISTER, @ref GENAVB_MSG_LISTENER_DEREGISTER, @ref GENAVB_MSG_LISTENER_RESPONSE, @ref GENAVB_MSG_LISTENER_STATUS, @ref GENAVB_MSG_TALKER_REGISTER, @ref GENAVB_MSG_TALKER_DEREGISTER, @ref GENAVB_MSG_TALKER_RESPONSE and @ref GENAVB_MSG_TALKER_STATUS,
* @ref GENAVB_MSG_VLAN_REGISTER, @ref GENAVB_MSG_VLAN_DEREGISTER and @ref GENAVB_MSG_VLAN_RESPONSE
* @ref GENAVB_MSG_ERROR_RESPONSE
* @ref GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE, @ref GENAVB_MSG_CLOCK_DOMAIN_RESPONSE, @ref GENAVB_MSG_CLOCK_DOMAIN_GET_STATUS and @ref GENAVB_MSG_CLOCK_DOMAIN_STATUS
* @ref GENAVB_MSG_GM_GET_STATUS, @ref GENAVB_MSG_GM_STATUS

Each message type has a corresponding message C structure (defined in detail in the GenAVB public headers) and
is usually valid on one specific control channel (with some exceptions). Messages are further divided into commands,
responses and indications. Commands/responses are used in pairs (with one response always received after a command is sent)
while indications can be sent/received at any time. The following table describes the mapping of message types to control
channels and their properties.

### Message types properties
Message type		|     Message structure 		| Control channel(s)		| Direction 	| Type
:-------------------	| ---------- 				| -				| - 		| -
Generic message type (1)| genavb_media_stack_msg			| GENAVB_CTRL_AVDECC_MEDIA_STACK	| - 		| -
MEDIA_STACK_CONNECT	| genavb_msg_media_stack_connect			| GENAVB_CTRL_AVDECC_MEDIA_STACK	| FROM stack	| Indication
MEDIA_STACK_DISCONNECT	| genavb_msg_media_stack_disconnect			| GENAVB_CTRL_AVDECC_MEDIA_STACK	| FROM stack	| Indication
Generic message type (1)| genavb_controller_msg			| GENAVB_CTRL_AVDECC_CONTROLLER	| - 		| -
Generic message type (1)| genavb_controlled_msg			| GENAVB_CTRL_AVDECC_CONTROLLED	| - 		| -
AECP (2)		| genavb_aecp_msg				| GENAVB_CTRL_AVDECC_CONTROLLER, GENAVB_CTRL_AVDECC_CONTROLLED	| TO/FROM stack | Any
ACMP_COMMAND (2)	| genavb_acmp_command			| GENAVB_CTRL_AVDECC_CONTROLLER	| FROM stack	| Command
ACMP_RESPONSE (2)	| genavb_acmp_response			| GENAVB_CTRL_AVDECC_CONTROLLER	| TO stack	| Response
ADP (2)			| genavb_adp_msg				| GENAVB_CTRL_AVDECC_CONTROLLER	| TO/FROM stack	| Any
Generic message type (1)| genavb_msg_msrp				| GENAVB_CTRL_MSRP			| -		| -
LISTENER_REGISTER	| genavb_msg_listener_register		| GENAVB_CTRL_MSRP			| TO stack	| Command
LISTENER_DEREGISTER	| genavb_msg_listener_deregister		| GENAVB_CTRL_MSRP			| TO stack	| Command
LISTENER_RESPONSE	| genavb_msg_listener_response		| GENAVB_CTRL_MSRP			| FROM stack	| Response
LISTENER_STATUS		| genavb_msg_listener_status		| GENAVB_CTRL_MSRP			| FROM stack	| Indication
TALKER_REGISTER		| genavb_msg_talker_register		| GENAVB_CTRL_MSRP			| TO stack	| Command
TALKER_DEREGISTER	| genavb_msg_talker_deregister		| GENAVB_CTRL_MSRP			| TO stack	| Command
TALKER_RESPONSE		| genavb_msg_talker_response		| GENAVB_CTRL_MSRP			| FROM stack	| Response
TALKER_STATUS		| genavb_msg_talker_status			| GENAVB_CTRL_MSRP			| FROM stack	| Indication
Generic message type (1)| genavb_msg_mvrp				| GENAVB_CTRL_MVRP			| -		| -
VLAN_REGISTER		| genavb_msg_vlan_register			| GENAVB_CTRL_MVRP			| TO stack	| Command
VLAN_DEREGISTER		| genavb_msg_vlan_deregister		| GENAVB_CTRL_MVRP			| TO stack	| Command
VLAN_RESPONSE		| genavb_msg_vlan_response			| GENAVB_CTRL_MVRP			| FROM stack	| Response
Generic message type (1)| genavb_msg_clock_domain			| GENAVB_CTRL_CLOCK_DOMAIN		| -		| -
CLOCK_DOMAIN_SET_SOURCE	| genavb_msg_clock_domain_set_source	| GENAVB_CTRL_CLOCK_DOMAIN		| TO stack	| Command
CLOCK_DOMAIN_RESPONSE	| genavb_msg_clock_domain_response		| GENAVB_CTRL_CLOCK_DOMAIN		| FROM stack	| Response
CLOCK_DOMAIN_GET_STATUS	| genavb_msg_clock_domain_get_status	| GENAVB_CTRL_CLOCK_DOMAIN		| TO stack	| Command
CLOCK_DOMAIN_STATUS	| genavb_msg_clock_domain_status		| GENAVB_CTRL_CLOCK_DOMAIN		| FROM stack	| Response/Indication
GM_GET_STATUS		| genavb_msg_gm_get_status		| GENAVB_CTRL_GPTP				| TO stack	| Command
GM_STATUS		| genavb_msg_gm_status			| GENAVB_CTRL_GPTP				| FROM stack	| Response/Indication
ERROR_RESPONSE (3)	| genavb_msg_error_response		| all				| FROM stack	| Response

(1) These message structures provide an easy way for the application to allocate a buffer large enough to receive any of the
supported message types for a given control channel.

(2) These messages encapsulate PDU's as defined in IEEE 1722.1-2011 and need further decoding to identify their specific type.

(3) This is a generic error response, valid in any control channel type.

A control channel is opened using the @ref genavb_control_open API. The API returns a handle that must be used in all other control API's.

Message reception by the application is done with the @ref genavb_control_receive function. This call is non-blocking, so
it will report an error if no message is available on the given control channel.

\if LINUX

On the Linux platform, a file descriptor
can be obtained for a control channel (using @ref genavb_control_rx_fd or @ref genavb_control_tx_fd), so regular system calls (poll, select, epoll...) may be used to make sure messages
are available.

\else

A callback can be registered using ::genavb_control_set_callback which is called when received data is available. The callback must not block and should notify another task which performs the actual data reception. After the callback has been called it must be re-armed by calling ::genavb_control_enable_callback. This should be done after the application has read all data available/written all data available (and never from the callback itself).

\endif

Sending a message can be performed in two different ways by the application:
* using @ref genavb_control_send : the application will post the message, and the call will return as soon as the message is sent without
waiting for any reply from the stack. If a response is expected, @ref genavb_control_receive should be called afterwards to fetch it.
There is no guarantee however that the first available message will be the response to the command just sent: other messages may
arrive in-between, so the application should make sure those are handled as well.
* using @ref genavb_control_send_sync : this call will post the message and wait for a response from the stack. This can be simpler 
for the application when a response is needed and it doesn't need to do anything else in the mean time, since the stack will take
care of providing the response message matching the command. Other messages can still be fetched normally afterwards using
@ref genavb_control_receive.

There are no restrictions on which channel types and message types can be used with each mode, so it is entirely up to the
application to use whichever mode makes more sense for its use cases.

To close a control channel use @ref genavb_control_close.

# Restrictions

Each control channel may be opened by a single application in the system. Also, if multiple threads send/receive from the same
control channel, locking should be provided by the application. Typically, responses to commands sent from thread X, can be received
by thread Y.

--

# Clock Domain control API

This API uses a @ref GENAVB_CTRL_CLOCK_DOMAIN control channel and interacts with the AVTP stack component. It allows to configure the
clock domain (@ref genavb_clock_domain_t) in a way close to IEEE 1722.1-2011 description.

### Clock domain

The clock domain can refer to an audio or a video clock domain. Any member of a clock domain is synchronous to the domain clock source
and is able to exchange media data with the other members.

The clock domain ID (@ref genavb_clock_domain_t) makes the link between the streams and the clock devices registered
to the media clock layer of genAVB stack. The clock device drivers are platform specific and documented in the platform specific
section.

Note: the domain ID is used by the media clock platform specific layer to find clock device drivers which have been registered.


### Setting the clock domain source

The application can set the clock source of the domain through the @ref GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE command with
@ref genavb_msg_clock_domain_set_source structure. This command receives the response @ref GENAVB_MSG_CLOCK_DOMAIN_RESPONSE with
@ref genavb_msg_clock_domain_response. The clock domain is considered configured and in a valid state only if status field of @ref
genavb_msg_clock_domain_response is equal to @ref GENAVB_SUCCESS.

_Important_: The clock domain needs to be configured before creating streams. If @ref GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE command
is used while active streams are configured, only a valid status from @ref genavb_msg_clock_domain_response guarantees that all
configured streams are in a valid state. If the return status is not successful, the clock domain is considered to be in an invalid
state and streaming is disabled. It is the application responsability to send an other @ref GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE
until a succesful response is received.

### Source types

Two top-level source types are available @ref genavb_clock_source_type_t

@ref GENAVB_CLOCK_SOURCE_TYPE_INTERNAL is used for an internal clock source. It is used for master endpoints which need to provide
their own timing information to the domain.
2 types of internal clocks are supported as defined in @ref genavb_clock_source_local_id_t :
* @ref GENAVB_CLOCK_SOURCE_AUDIO_CLK
The timing source is the physical audio clock of the audio codec associated to the domain. This source type requires a generation 
 clock device associated to the audio codec clock with HW support. The association between a domain ID and the audio codec is
 described in the platform specific section. It is used when the media frames are played/captured to/from a HW audio device.
* @ref GENAVB_CLOCK_SOURCE_PTP_CLK
The timing source is a perfect gPTP based clock generated in software. It is generally used when the media frames are played/captured
to/from a file. This clock source has a clock device which is not associated to a given domain ID.

@ref GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM is used when the clock source is a remote stream. The remote stream can be any listener stream of
any format. It is used by slave endpoints which need to synchronise their clock domain to a remote master. This source type requires a
recovery clock device associated to the audio codec. The association between a domain ID and the audio codec is described in the
platform specific section.


### Clock source types summary
Clock source type		|    Clock source type local ID		| Requirement			| Typical use case
:-------------------	        | ---------- 				| -----------			|-----------------
GENAVB_CLOCK_SOURCE_TYPE_INTERNAL  | GENAVB_CLOCK_SOURCE_AUDIO_CLK		| Generation clock device	| Master, audio codec
GENAVB_CLOCK_SOURCE_TYPE_INTERNAL	| GENAVB_CLOCK_SOURCE_PTP_CLK		| None (software)		| Master, slave, file server
GENAVB_CLOCK_SOURCE_TYPE_STREAM	|					| Recovery clock device		| Slave, audio codec


### Indications

It is possible to get status on the clock domain by listening @ref GENAVB_MSG_CLOCK_DOMAIN_STATUS indications
(using @ref genavb_control_receive).

Note: These messages are in beta state.

--

# MSRP control API

This API uses a @ref GENAVB_CTRL_MSRP control channel and interacts with the SRP stack component. It allows a Talker/Listener
application to make stream reservations on an AVB network (through SRP as specificed in 802.1Qat-2010). The API also allows
the application to be notified of existing reservations (made by other endpoints) and their status. If the AVDECC stack component
is enabled, stream reservations are done automatically (for streams connected through AVDECC) and it's not required for the
application to use this API.

### Talker streams

A Talker can register streams on the network through the @ref GENAVB_MSG_TALKER_REGISTER command. To deregister a stream the
@ref GENAVB_MSG_TALKER_DEREGISTER command is used. Both commands receive the same response @ref GENAVB_MSG_TALKER_RESPONSE.

To learn about the existing Listeners for registered Talker streams, the application can listen to @ref GENAVB_MSG_TALKER_STATUS
indications (using @ref genavb_control_receive).

### Listener streams

A Listener can register the streams it wishes to receive through the @ref GENAVB_MSG_LISTENER_REGISTER command. To deregister
a stream the @ref GENAVB_MSG_LISTENER_DEREGISTER command is used. Both commands receive the same response @ref GENAVB_MSG_LISTENER_RESPONSE.

To learn about the Talker status for registered Listener streams, the application can listen to @ref GENAVB_MSG_LISTENER_STATUS
indications (using @ref genavb_control_receive).

--

# GPTP control API

This API uses a @ref GENAVB_CTRL_GPTP control channel and interacts with the GPTP stack component. It allows a Talker/Listener
application to retrieve the status of the GPTP Grand Master. The API also allows the application to be notified of Grand Master status changes.

--

# AVDECC control API

The API has been designed to expose most of the AVDECC functionality to a user-space application, while hiding out protocol
interaction details.

It can be used by an AVDECC controller to:
* connect and disconnect streams,
* get information about AVDECC entities,
* send AECP commands and receive responses.

It can be used by an application on an AVDECC talker or listener entity to:
* receive stream connection/disconnection messages,
* receive AECP commands and send responses.

![](avdecc_control.png)

---

## Exchanges on an GENAVB_CTRL_AVDECC_MEDIA_STACK channel
This channel can be used by a media stack application to be notified about stream connections/disconnections.

When a stream is connected, the stack will send a @ref GENAVB_MSG_MEDIA_STACK_CONNECT message, whose content maps to the @ref genavb_stream_params structure.
It normally contains all the necessary parameters to initialize the media stack. It is also the same structure that is required
by the @ref genavb_stream_create function to create an AVTP stream, so it may be passed as is by the media stack application.

When a stream is disconnected, the stack will send a @ref GENAVB_MSG_MEDIA_STACK_DISCONNECT message, mapping the @ref genavb_msg_media_stack_disconnect structure.

> The @ref GENAVB_CTRL_AVDECC_MEDIA_STACK channel should not be confused with the stream creation API: it is only used to receive stream connections/disconnections
messages received through the AVDECC protocol, and it is then the responsibility of the application to actually request creation/destruction of the stream
in the AVTP component of the stack, using the @ref genavb_stream_create and @ref genavb_stream_destroy functions.


---

## ACMP exchanges on an GENAVB_CTRL_AVDECC_CONTROLLER channel
ACMP messages can be used by a controller to request the connection or disconnection of AVTP streams on the AVB network, and to get information about the existing streams.

ACMP commands may be sent by a controller application through the @ref GENAVB_CTRL_AVDECC_CONTROLLER channel, using the @ref GENAVB_MSG_ACMP_COMMAND message type and
the @ref genavb_acmp_command structure. Only commands that a controller entity may send (according the specification) are allowed:
* @ref ACMP_CONNECT_RX_COMMAND ,
* @ref ACMP_DISCONNECT_RX_COMMAND ,
* @ref ACMP_GET_TX_STATE_COMMAND ,
* @ref ACMP_GET_RX_STATE_COMMAND ,
* @ref ACMP_GET_TX_CONNECTION_COMMAND .

The stack will post the ACMP responses on that same channel, using the @ref GENAVB_MSG_ACMP_RESPONSE message type and the @ref genavb_acmp_response structure.

The content of the @ref genavb_acmp_command and @ref genavb_acmp_response structures closely matches the ACMP specification, so the AVDECC standard should be
referred to for additional details. To simplify the task of the application however, the GenAVB stack will handle all the network-related tasks, such as retransmission on timeout.

> Since there are no notification mechanisms in the ACMP specification, the stack will never send any ACMP messages to the application without receiving an ACMP command from the application first.


---

## ADP exchanges on an GENAVB_CTRL_AVDECC_CONTROLLER channel
Handling ADP messages will allow a controller application to maintain an up-to-date view of the AVDECC entities discovered on the AVB network by the GenAVB stack.

ADP messages use the @ref GENAVB_MSG_ADP message type and the @ref genavb_adp_msg structure.

Each time an entity becomes available on or departs the network, the stack will send an ADP message to the application, with the msg_type field of the @ref genavb_adp_msg structure
set respectively to @ref ADP_ENTITY_AVAILABLE or @ref ADP_ENTITY_DEPARTING. The info field of that structure will contain additional details on the entity. ADP_ENTITY_AVAILABLE messages
will also be sent by the stack if the entity was updated in any way, but not for the simple refresh messages sent periodically by the remote entity.
The stack however will age the entities according to the (non)-received messages, and it remove entites that are no longer visible from its database. When this happens, the application will also be notified of the departing entity.

The application may also request specific information by sending ADP messages to the stack:
* Messages from the application with the msg_type (of the @ref genavb_adp_msg structure) set to ADP_ENTITY_DISCOVER will trigger a dump of all currently discovered entities.
  Each discovered entity will be notified to the application with its own @ref ADP_ENTITY_AVAILABLE message (one single entity per message).
  This can be useful on startup for the application to sync its state with that of the stack.
  The total field can be used by the application to determine when its view of the network is complete (i.e. when it has received information about total different entities).
* To get information about a specific entity, the application shall set msg_type to @ref ADP_ENTITY_AVAILABLE, info.entity_id to the AVDECC id of the requested entity, with the
  other fields being unused. The stack will reply with the corresponding entity if it exists.

If no entity was found or if an invalid msg_type was used, a reply will be sent by the stack to the application with msg_type set to @ref ADP_ENTITY_NOTFOUND.


---

## AECP exchanges
AECP communication may happen through either the @ref GENAVB_CTRL_AVDECC_CONTROLLER or @ref GENAVB_CTRL_AVDECC_CONTROLLED channels, depending on the type of
the entity sending/receiving the messages.
A controller can use AECP messages to request specific information about an entity, or request actions to be performed by the entity.
A talker or Listener application can use AECP messages to reply to commands from a controller (such as get/set volume, etc), and also notify the stack of changes
within the entity (such as a volume change event from outside AVDECC), so that the stack can then forward the message to
interested entities (usually controllers registered for unsolicited notifications).

AECP messages use the @ref GENAVB_MSG_AECP message type and the @ref genavb_aecp_msg structure. Of all the AECP types, only the AEM ones are
currently supported by the stack. Because of the breadth of the AECP AEM protocol, the various AECP AEM PDUs are mapped as is into the buf
field of the @ref genavb_aecp_msg structure. Additional information such as the AECP msg_type and AECP status is also available in that structure.
Additional details about those fields and the format of AECP AEM PDUs may be found in the AVDECC standard. To simplify the task of the
application however, the GenAVB stack will handle all the network-related tasks, such as retransmission on timeout and controller locking/availability checks.
A controller application can therefore simply send commands (and possibly wait for a response), without having to worry about other details.
Similarly, a talker or listener may simply send messages to the stack when it needs to, without worrying about potential network protocol issues.


### AEM model support
The AEM models to be used by the stack are currently defined and generated at compile-time through the aem-manager host application, whose source code
is provided in the apps/linux folder.The GenAVB stack will load AEM model files at run-time (based on command-line parameters for the Linux platform).
More than one AEM model may be loaded, will the following constraints:
* at most one controller entity may be declared per endpoint,
* at most one non-controller entity (talker or listener) may be declared per endpoint.

This means in practice, only up to 2 entities may be declared per endpoint. Those limitations might be lifted in future versions of the stack.

Dynamically updating the AEM model at run-time is also not possible currently, but changing the value(s) stored in a CONTROL descriptor is. The stack maintains its own view of the whole AEM model, so
that ::AECP_AEM_CMD_GET_CONTROL commands can be replied to without involving the application. However, the application _has to_ notify the stack of changes in the values stored in a CONTROL descriptor, by
sending the relevant ::AECP_AEM_CMD_SET_CONTROL unsolicited response in reaction to a change made outside AVDECC. For consistency however the stack will never update the values immediately upon reception of
a ::AECP_AEM_CMD_SET_CONTROL command from a controller: this ensures the application can perform additional validation steps on the requested value and report any issue back to the stack.In conclusion, CONTROL
descriptor values are updated either by the value received from a controller in the ::AECP_AEM_CMD_SET_CONTROL command (once stack validate it on reception and application respond with ::AECP_AEM_SUCCESS
in ::genavb_aecp_msg.status) or by a valid value from a ::AECP_AEM_CMD_SET_CONTROL unsolicited response sent by application.

> Only 2 value types are currently fully supported by the stack for CONTROL descriptors: LINEAR_UINT8 and UTF8.


### AECP exchanges on an GENAVB_CTRL_AVDECC_CONTROLLER channel
On such a channel:
* Only commands shall be sent by the application (::genavb_aecp_msg.msg_type == ::AECP_AEM_COMMAND),
* Only responses will be sent by the stack and received by the application (::genavb_aecp_msg.msg_type == ::AECP_AEM_RESPONSE).

::aecp_pdu.controller_entity_id, ::aecp_pdu.sequence_id will be overwritten by the stack before sending the message on the network and can be safely ignored by the application.
The stack will check the entity ID (::aecp_pdu.entity_id) provided by the application, and:
* If it is 0, it will send the command to the first discovered entity (talker or listener),
* Otherwise, it will make sure the entity is currently available (based on the database of entities maintained by the ADP component of the stack), and send the command to it.
The stack will report an error if no entity could be found.

Aside from the entity ID check and network protocol handling, the stack acts mostly as a pass-through in this case, and the commands/responses will be sent from/to the application without any modification.


### AECP exchanges on an GENAVB_CTRL_AVDECC_CONTROLLED channel
On such a channel:
* Only responses shall be sent by the application,
* Only commands will be sent by the stack and received by the application.

Not all commands are supported by the stack for network reception yet. When possible, commands are also handled directly without disturbing the application. As a result, only a
limited set of commands may be received by the application. The table below lists the commands currently supported by the stack for reception from the network for talker or listener entities.
Command                                          | Handled by the stack | Passed to the application
 ---------------------------                     | :------------------: | :-----------:
AECP_AEM_CMD_READ_DESCRIPTOR                     | Y                    | N
AECP_AEM_CMD_ACQUIRE_ENTITY                      | Partial*             | N
AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION   | Y                    | N
AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION | Y                    | N
AECP_AEM_CMD_SET_CONTROL                         | Y                    | Y
AECP_AEM_CMD_GET_CONTROL                         | Y                    | N
AECP_AEM_CMD_START_STREAMING                     | Y                    | Y
AECP_AEM_CMD_STOP_STREAMING                      | Y                    | Y
Others                                           | N                    | N

> *The ACQUIRE_ENTITY command is implemented, with the following limitations:
> * No CONTROLLER_AVAILABLE checks are being performed in this case
> * The acquiring controller will not be sent unsolicited responses as mandated by the AVDECC specification, unless it did register for unsolicited notifications as well.
> * Only whole entities can be acquired, and not other descriptors.


#### Handling of SET_CONTROL command
When receiving a ::AECP_AEM_CMD_SET_CONTROL command from the network, the stack will (in that order):
* Make sure the requested CONTROL descriptor can be found in the AEM model loaded in memory and is not read-only,
* Make sure the requested value is valid,
* Pass the command to the application,
* Send IN_PROGRESS responses to the requesting controller until the application provides a response.In case the application does not respond with a proper msg after
sending AECP_CFG_MAX_AEM_IN_PROGRESS messages (A total of 10 seconds period) the stack will send a ::AECP_AEM_CMD_SET_CONTROL failure response.

#### Handling of START_STREAMING and STOP_STREAMING commands
When receiving a START_STREAMING or STOP_STREAMING command from the network, the stack will (in that order):
* Make sure the requested STREAM descriptor can be found in the AEM model loaded in memory and is not read-only,
* Pass the command to the application,
* Send IN_PROGRESS responses to the requesting controller until the application provides a response.


#### Responses from the application
A talker or listener application may send responses through an @ref GENAVB_CTRL_AVDECC_CONTROLLED channel at any time. The entity ID field of the response message coming from
the application will be ignored: the stack will determine the entity the response should originate from on its own, under the assumption that only one non-controller entity is
declared on the endpoint (only one non-controller entity per endpoint is currently supported). If no non-controller entity has been declared on the endpoint, the message will
be ignored. If the response is not related to an AECP command previously received by the application, the U bit (in aecp_aem_pdu.u_command_type) shall be set to 1 by the application, and
the ::aecp_pdu.controller_entity_id, ::aecp_pdu.sequence_id fields will be ignored by the stack. Otherwise (the response is in reply to a received command), the U bit shall be set to 0 and
the ::aecp_pdu.controller_entity_id, ::aecp_pdu.sequence_id fields shall match the value from the command message.

After receiving a (response) message from the application and finding the corresponding entity, the stack will (in that order):
* For ::AECP_AEM_CMD_SET_CONTROL commands only:
  * make sure the CONTROL descriptor exists in the AEM model of the entity
  * On a Response with ::AECP_AEM_SUCCESS in ::genavb_aecp_msg.status:
    - If the U bit **is not** set in aecp_aem_pdu.u_command_type (A regular ::AECP_AEM_RESPONSE): update the CONTROL descriptor with the value received in the command message into the
      descriptor and send back a response with it.
    - If the U bit **is set** in aecp_aem_pdu.u_command_type (An unsolicited ::AECP_AEM_RESPONSE): validate the new value coming from the application and update the CONTROL descriptor with the new value.
  * On a Response **with other than** ::AECP_AEM_SUCCESS in ::genavb_aecp_msg.status: Send response with the current value in the descriptor

* If the U bit is **not** set in aecp_aem_pdu.u_command_type, try and find a match between the response and a previously received command, based on ::aecp_pdu.controller_entity_id and ::aecp_pdu.sequence_id.
  If a previously received command is found, the response, with values from the descriptor, will be sent to the originating controller entity, and the stack will stop sending IN_PROGRESS responses for that command.Otherwise (no previously received command is found), the response is not sent and no update is performed. If the U bit is set, those checks will be skipped.
* If the command the response is for is subject to unsolicited notifications, loop through all controllers registered for unsolicited notifications, and send an unsolicited response to them.


---


