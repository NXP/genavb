Frame Preemption API usage {#frame_preemption_usage}
======================================

This API is used to configure the frame preemption feature (as defined in IEEE 802.3br-2016 and IEEE 802.1Qbu-2016) of a given network port.
Frame preemption allows Express traffic to interrupt on-going Preemptable traffic. Preemption can be enabled or disabled at the port level.
Frame priorities can be configured as either Preemptable or Express.

This feature requires specific hardware support and returns an error if the corresponding network port doesn't support it.

The 802.1Q configuration is set using ::genavb_fp_set:
* port_id: the logical port ID.
* type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_1Q.
* config: the ::genavb_fp_config configuration. Only the write members of the ::genavb_fp_config_802_1Q union should be set.

Frame priorities mapped to the same Traffic class must have the same Preemptable/Express setting.

The 802.1Q configuration is retrieved using ::genavb_fp_get:
* port_id: the logical port ID.
* type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_1Q.
* config: the ::genavb_fp_config configuration. Only the read members of the ::genavb_fp_config_802_1Q union will be set when the function returns.

The 802.3 configuration is set using ::genavb_fp_set:
* port_id: the logical port ID.
* type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_3.
* config: the ::genavb_fp_config configuration. Only the write members of the ::genavb_fp_config_802_3 union should be set.

If verification is enabled (verify_disable_tx == false), only after the verification process is complete successfully (status_verify == ::GENAVB_FP_STATUS_VERIFY_SUCCEEDED), is frame preemption enabled in the port.

In the current release, Frame Preemption can not be enabled at the same time as Scheduled Traffic.


The 802.3 configuration is retrieved using ::genavb_fp_get:
* port_id: the logical port ID.
* type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_3.
* config: the ::genavb_fp_config configuration. Only the read members of the ::genavb_fp_config_802_3 union will be set when the function returns.
