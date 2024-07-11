Scheduled Traffic API usage {#scheduled_traffic_usage}
======================================

This API is used to configure the scheduled traffic feature (as defined in IEEE 802.1Q-2018 Section 8.6.9) of a given network port.
Scheduled traffic allows the configuration of a sequence of gate states per port which determine the set of traffic classes that are allowed to transmit at any given time.

This feature requires specific hardware support and returns an error if the corresponding network port doesn't support it.

The administrative configuration is set using ::genavb_st_set_admin_config. 
Parameters:
* port_id: the logical port ID.
* clk_id: the ::genavb_clock_id_t clock ID domain. The times provided in the configuration are based on this clock reference.
* config: the ::genavb_st_get_config configuration 
    + enable: 0 or 1, if 0 scheduled traffic is disabled
    + base_time (nanoseconds): the instant defining the time when the scheduling starts. If base_time is in the past, the scheduling will start when base_time + (N * cycle_time_p / cycle_time_q) is greater than "now". (N being the smallest integer making the equation true)
    + cycle_time_p and cycle_time_q (seconds): the scheduling cycle time in rational format. It is the time when the list should be repeated. If the provided list is longer than the cycle time, the list will be truncated.
    + cycle_time_ext (nanoseconds): the amount of time that the current gating cycle can be extended when a new cycle configuration is configured.  
    + control_list: the ::genavb_st_gate_control_entry control list (see description below)

Control list description:
* it is an array of ::genavb_st_gate_control_entry elements of list_length length.
* operation: the gate operation ::genavb_st_operations_t. (Note: only ::GENAVB_ST_SET_GATE_STATES is supported)
* gate_states: a bit mask in which the bit in position N refers to the traffic class N. If the bit is set, the traffic class is allowed to transmit, if the bit is not set the traffic class is not allowed to transmit.
* time_interval (nanoseconds): the duration of the state defined by operation and gate_states before moving to the next entry. 
