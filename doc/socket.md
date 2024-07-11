Socket API usage {#socket_usage}
======================================

The socket API allows the creation of a network endpoint and exchanging layer 2 network frames. 
Transmit and receive sockets are handled separately and each of them requires its own (unidirectional) socket.
Parameters are set when the socket is opened and cannot be changed afterward.

# Socket transmit {#sock_tx}

Socket is created using ::genavb_socket_tx_open.

Frames are transmitted using ::genavb_socket_tx function.
The function expects a continuous buffer including layer 2 header if ::GENAVB_SOCKF_RAW is set and excluding layer 2 header if not. The frame is copied before the function returns so that the provided buffer can be freed. A success return code means that the frame has been correctly queued.

Two transmit modes are available and are set using ::genavb_sock_f_t flag:
* If ::GENAVB_SOCKF_RAW is set, it's the caller responsability to set the layer 2 header. The l2 field of ::net_address is not used. 
* If ::GENAVB_SOCKF_RAW isn't set, the layer 2 header is add internally based l2 field of ::net_address and inserted automatically.

Whatever the transmit mode, source MAC address and VLAN are overwritten by the stack.
The socket networking parameters are set in ::genavb_socket_tx_params addr field. See @ref net_addr for a complete description. Only ::PTYPE_L2 protocol type is supported in transmit.

The socket can be closed and its associated resources freed using ::genavb_socket_tx_close.

# Socket receive {#sock_rx}

Socket is created using ::genavb_socket_rx_open.

Frames are received by calling ::genavb_socket_rx, the buffer length must be at least the interface MTU size. The frames are copied to the provided buffer which needs to be continous.

Two receive modes are available and are set using ::genavb_sock_f_t flag:
* If ::GENAVB_SOCKF_NONBLOCK is set, the socket is configured in non-blocking mode and the call to ::genavb_socket_rx will never block. If no frame is available for reading, the API returns ::GENAVB_ERR_SOCKET_AGAIN.
* If ::GENAVB_SOCKF_NONBLOCK is not set, the socket is configured in blocking mode and the call to ::genavb_socket_rx only returns if a frame has been received or because an error occured. To use the blocking mode a task needs to be dedicated to receive handling. (RTOS only)


The socket networking parameters are set in ::genavb_socket_rx_params addr field. See @ref net_addr for a complete description.
The following receive protocol types are available:
* ::PTYPE_L2: the frame matching is performed using the VLAN ID and the destination MAC address. The other fields of ::net_address are not used. 
* ::PTYPE_OTHER: the socket will receive all the frames which have not been received by another genAVB socket. This protocol type is generally used to connect a generic TCP/IP protocol stack.

The socket can be closed and its associated resources freed using ::genavb_socket_rx_close.

# Flow control (non-blocking mode) {#flow_control_sock}

In receive, two approaches are possible:
* the stream is periodic like isochronous time-sensitive traffic. The application is aware of the frames timing and therefore can just "poll" the socket at the expected time.

* the stream has no known timing and the application needs to be notified when frames are available for read.

In transmit, a time sensitive application expects being able to send its frames at a defined timing. If not it's generally a critical error and the send will  be retried. Nonetheless, for some OS, some APIs may be available to be notified when buffer space is available for transmit.

\if LINUX

Using the file descriptor returned by the ::genavb_socket_rx_fd and ::genavb_socket_rx_fd  functions, an application may call poll/epoll/select system calls to sleep and be woken up only when received data is available or when buffer space is available for data to transmit.

\else

A callback can be registered using ::genavb_socket_rx_set_callback which is called when received data is available. The callback must not block and should notify another task which performs the actual data reception. After the callback has been called it must be re-armed by calling ::genavb_socket_rx_enable_callback. This should be done after the application has read all data available/written all data available (and never from the callback itself).

\endif

# Network address {#net_addr}

The ::net_address addr field is used to configure the socket networking parameters:
* ptype: the protocol type. Needs to be set to ::PTYPE_L2 or ::PTYPE_OTHER
* port: the logical port number
* vlan_id: the VLAN ID in network order (can be set to ::VLAN_VID_NONE if no vlan is required)
* priority: traffic priority (range 0 to 7) (only used in transmit)
* u.l2: the l2 address 
  + dst_mac: destination MAC address
  + protocol: the ether type (only used in transmit)
