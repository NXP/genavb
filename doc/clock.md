Clock API usage {#clock_usage}
======================================

The clock API offers a high-level API to retrieve the current time for various clocks.

* ::GENAVB_CLOCK_MONOTONIC is based on a monotonic system time and should be used when a continous time is needed.
* ::GENAVB_CLOCK_GPTP_0_0 and ::GENAVB_CLOCK_GPTP_0_1 are gPTP clocks. Respectively domain 0 and 1 for interface 0
* ::GENAVB_CLOCK_GPTP_1_0 and ::GENAVB_CLOCK_GPTP_1_1 are gPTP clocks. Respectively domain 0 and 1 for interface 1

Specificities of the time returned by gPTP clocks:
* it has no guarantee to be monotonic and discontinuities may happen
* it is not guaranteed to be synchronized to gPTP Grandmaster. Making sure local system is well synchronized to the gPTP Grandmaster is out-of-scope of this API.

The current time is retrieved using ::genavb_clock_gettime64 function which provides a 64 bits nanoseconds time value.
