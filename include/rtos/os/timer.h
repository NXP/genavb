/*
 * Copyright 2019-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public API
 \details OS specific timer API definition for the GenAVB/TSN library

 \copyright Copyright 2019-2021, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_TIMER_API_H_
#define _OS_GENAVB_PUBLIC_TIMER_API_H_

/**
 * \defgroup timer			Timer
 * \ingroup library
 *
 * The timer API provides lightweight and responsive timers which are designed for real-time use cases.
 * Each timer has a dedicated hardware ressource and hence should only be used when precise timing is required. For low resolution timers it's preferable to use OS timers.
 *
 * First the timer needs to be created using ::genavb_timer_create function. This guarantees that the needed hardware ressources are available and reserved. The callback needs to be registered separately using ::genavb_timer_set_callback. The callback is called in interrupt context when the timer reaches its expiration time and can be called in task context when errors are reported.
 *
 * Then the timer is started using ::genavb_timer_start and can be stopped using ::genavb_timer_stop. The timer API supports one-shot and periodic operations.
 *
 * If discontinuities happen for gPTP clocks, the callback returns immediately (with negative count argument). In this case it's needed to restart the timer.
 *
 * Finally, timer can be fully freed using ::genavb_timer_destroy.
 *
 * # PPS support {#pps}
 *
 * Pulse Per Second feature (PPS) is supported by the timer API by allowing requesting of a timer which has been identified at a lower level to have an available signal output. A PPS timer triggers a pulse signal at each expiration, but otherwise has the same behavior as a regular timer. Currently only one PPS timer can be available. Note that there is no restriction for the timer period contrary to what the PPS term may suggest.
 *
 * The PPS timer can be requested by using the ::GENAVB_TIMERF_PPS flag. A succesful call to ::genavb_timer_create using this flag guarantee the PPS timer availability and ownership. Then the flag needs to be set again in ::genavb_timer_start to trigger the output signal. If not set, the timer is started as a regular timer.
 *
 */

/** Allocate/create a new timer
 * \ingroup	timer
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param t	pointer to the struct genavb_timer context
 * \param id	clock id
 * \param flags the following flags may be bitwise ORed:\n
 *		::GENAVB_TIMERF_PPS request a timer with PPS signal output support.
 */
int genavb_timer_create(struct genavb_timer **t, genavb_clock_id_t id, genavb_timer_f_t flags);

/** Set the timer callback
 * \ingroup		timer
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param t		pointer to the struct genavb_timer context
 * \param callback	callback function pointer. Its arguments are the callback data as
 *			provided in this API and count which is the number of times the timer
 *			expired and if negative indicates a clock discontinuity. In this case,
 *			the timer has been stopped and needs to be re-started.
 * \param data		callback data
 */
int genavb_timer_set_callback(struct genavb_timer *t,
			      void (*callback)(void *data, int count),
			      void *data);

/** Setup and start a timer created by genavb_timer_create()
 *  If the timer is already running it is re-started with new period.
 *  For periodic timers, the first expiration happens at the end of the first
 *  period (value + interval).
 * \ingroup		timer
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param t		pointer to the OS-dependent struct os_timer to start
 * \param value		expiration time in nanoseconds (for a one shot timer).
 * 			Start of the first period (for a periodic timer), usually 0.
 * \param interval	periodic timer value in nanoseconds . If zero, the timer is one shot.
 * \param flags 	the following flags may be bitwise ORed:\n
 * 			::GENAVB_TIMERF_PPS enable the PPS signal output. The timer needs to have been
 *			successfully created with the same flag.\n
 *			::GENAVB_TIMERF_ABS the "value" argument is provided in absolute time.
 *			If not set the time is relative.
 */
int genavb_timer_start(struct genavb_timer *t, uint64_t value, uint64_t interval, genavb_timer_f_t flags);

/** Stop a running timer
 *  The function can be safely called if the timer is already stopped.
 * \ingroup	timer
 * \param t	pointer to the genavb_timer to stop
 */
void genavb_timer_stop(struct genavb_timer *t);

/** Destroy/free a timer
 *  If the timer is still running it will be stopped first.
 * \ingroup	timer
 * \param t	pointer to the genavb_timer to destroy
 */
void genavb_timer_destroy(struct genavb_timer *t);

#endif /* _OS_GENAVB_PUBLIC_TIMER_API_H_ */
