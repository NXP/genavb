Timer API usage {#timer_usage}
======================================

The timer API provides lightweight and responsive timers which are designed for real-time use cases.
Each timer has a dedicated hardware ressource and hence should only be used when precise timing is required. For low resolution timers it's preferable to use OS timers. 

First the timer needs to be created using ::genavb_timer_create function. This guarantees that the needed hardware ressources are available and reserved. The callback needs to be registered separately using ::genavb_timer_set_callback. The callback is called in interrupt context when the timer reaches its expiration time and can be called in task context when errors are reported. 

Then the timer is started using ::genavb_timer_start and can be stopped using ::genavb_timer_stop. The timer API supports one-shot and periodic operations. 

If discontinuities happen for gPTP clocks, the callback returns immediately (with negative count argument). In this case it's needed to restart the timer.

Finally, timer can be fully freed using ::genavb_timer_destroy.

# PPS support {#pps}

Pulse Per Second feature (PPS) is supported by the timer API by allowing requesting of a timer which has been identified at a lower level to have an available signal output. A PPS timer triggers a pulse signal at each expiration, but otherwise has the same behavior as a regular timer. Currently only one PPS timer can be available. Note that there is no restriction for the timer period contrary to what the PPS term may suggest.

The PPS timer can be requested by using the ::GENAVB_TIMERF_PPS flag. A succesful call to ::genavb_timer_create using this flag guarantee the PPS timer availability and ownership. Then the flag needs to be set again in ::genavb_timer_start to trigger the output signal. If not set, the timer is started as a regular timer.
