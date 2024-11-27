/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file net_port_netc_1588.c
 @brief RTOS specific Timer service implementation
 @details
*/

#include "common/types.h"
#include "common/log.h"
#include "clock.h"

#include "net_port_netc_1588.h"
#include "msgintr.h"

#include "config.h"

#if CFG_NUM_NETC_HW_CLOCK > 0

#include "fsl_netc_timer.h"
#include "hw_clock.h"
#include "hw_timer.h"

#define TIMER_MSG_DATA		0U
#define NETC_CLOCK_MAX_TIMER	4
#define FIPER_PRESCALE		24000

struct netc_1588_timer {
	uint8_t index; /* Timer index */
	bool enabled;
	bool irq_enabled;
	bool is_pps; /* Check if timer supports 1PPS signal generation */
	unsigned int id; /* Timer source like Alarm, Fiper */
	netc_timer_irq_flags_t irq_flag;
	struct hw_timer hw_timer;
};

struct msgintr {
	MSGINTR_Type *base;
	uint8_t channel;
};

/* Global context for 1588_timer */
#define NETC_MAX_HW_CLOCK	2 /* Freerunning + Synchronized */

struct netc_hw_clock {
	unsigned int id;
	bool owner;
	struct hw_clock clock[NETC_MAX_HW_CLOCK];
	int32_t ppb;
	netc_timer_handle_t timer_handle;
	uint8_t timer_n; /* Store the timers in use for a specific clock */
	struct netc_1588_timer timer[NETC_CLOCK_MAX_TIMER];
	uint32_t gclk_period; /* generated clock period */
	uint32_t tclk_period; /* Timer clock period */
	struct msgintr msgintr;
};

static struct netc_hw_clock hw_clock[CFG_NUM_NETC_HW_CLOCK] =
{
	[0] = {
		.id = BOARD_NETC_HW_CLOCK_0_ID,
		.owner = BOARD_NETC_HW_CLOCK_0_OWNER,

		.clock = {
			[0] = {
				/* Synchronized clock */
				.rate = NSECS_PER_SEC,
				.period = 0, /* 1 << 64 */
				.to_ns = {
					.shift = 0,
				},
				.to_cyc = {
					.shift = 0,
				},
			},
			[1] = {
				/* FreeRunning clock */
				.period = 0, /* 1 << 64 */
				.to_ns = {
					.shift = 29,
				},
				.to_cyc = {
					.shift = 32,
				},
			},
		},

		#ifdef BOARD_HW_CLOCK0_NUM_TIMERS

		.timer_n = BOARD_HW_CLOCK0_NUM_TIMERS,

		.timer = {
		#if BOARD_HW_CLOCK0_NUM_TIMERS > 0
			[0] = {
				.id = BOARD_HW_CLOCK0_TIMER0_ID,
				#ifdef BOARD_HW_CLOCK0_TIMER0_ENABLED
				.enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER0_IRQ_ENABLED
				.irq_enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER0_PPS
				.is_pps = true,
				#endif
			},
		#endif

		#if BOARD_HW_CLOCK0_NUM_TIMERS > 1
			[1] = {
				.id = BOARD_HW_CLOCK0_TIMER1_ID,
				#ifdef BOARD_HW_CLOCK0_TIMER1_ENABLED
				.enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER1_IRQ_ENABLED
				.irq_enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER1_PPS
				.is_pps = true,
				#endif
			},
		#endif

		#if BOARD_HW_CLOCK0_NUM_TIMERS > 2
			[2] = {
				.id = BOARD_HW_CLOCK0_TIMER2_ID,
				#ifdef BOARD_HW_CLOCK0_TIMER2_ENABLED
				.enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER2_IRQ_ENABLED
				.irq_enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER2_PPS
				.is_pps = true,
				#endif
			},
		#endif

		#if BOARD_HW_CLOCK0_NUM_TIMERS > 3
			[3] = {
				.id = BOARD_HW_CLOCK0_TIMER3_ID,
				#ifdef BOARD_HW_CLOCK0_TIMER3_ENABLED
				.enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER3_IRQ_ENABLED
				.irq_enabled = true,
				#endif
				#ifdef BOARD_HW_CLOCK0_TIMER3_PPS
				.is_pps = true,
				#endif
			},
		#endif

		#if BOARD_HW_CLOCK0_NUM_TIMERS > 4 || BOARD_HW_CLOCK0_NUM_TIMERS < 0
		#error `BOARD_HW_CLOCK0_NUM_TIMERS` value is not supported
		#endif

		},
		#endif

		#ifdef BOARD_NETC_HW_CLOCK_0_MSGINTR_BASE
		.msgintr = {
			.base = BOARD_NETC_HW_CLOCK_0_MSGINTR_BASE,
			.channel = BOARD_NETC_HW_CLOCK_0_MSGINTR_CH,
		}
		#endif
	}
#if CFG_NUM_NETC_HW_CLOCK > 1
#error
#endif
};

enum {
	NETC_TIMERID_ALARM1 = 0,
	NETC_TIMERID_ALARM2 = 1,
	NETC_TIMERID_FIPER1 = 2,
	NETC_TIMERID_FIPER2 = 3,
	NETC_TIMERID_FIPER3 = 4,
	NETC_TIMERID_MAX = 5
};

static const uint32_t netc_timer_id[NETC_TIMERID_MAX] = {
	[NETC_TIMERID_ALARM1] = kNETC_TimerAlarm1,
	[NETC_TIMERID_ALARM2] = kNETC_TimerAlarm2,
	[NETC_TIMERID_FIPER1] = kNETC_TimerFiper1,
	[NETC_TIMERID_FIPER2] = kNETC_TimerFiper2,
	[NETC_TIMERID_FIPER3] = kNETC_TimerFiper3,
};

static const uint32_t netc_timer_irq_flag[NETC_TIMERID_MAX] = {
	[NETC_TIMERID_ALARM1] = kNETC_TimerAlarm1IrqFlag,
	[NETC_TIMERID_ALARM2] = kNETC_TimerAlarm2IrqFlag,
	[NETC_TIMERID_FIPER1] = kNETC_TimerFiper1IrqFlag,
	[NETC_TIMERID_FIPER2] = kNETC_TimerFiper2IrqFlag,
	[NETC_TIMERID_FIPER3] = kNETC_TimerFiper3IrqFlag,
};

uint64_t netc_1588_hwts_to_u64(struct hw_clock *clk, uint32_t hwts_ns)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)clk->priv;
	uint64_t hw_time;
	uint64_t nanoseconds = 0;

	hw_time = clk->read_counter(hw_clock);

	nanoseconds = (hw_time & 0xffffffff00000000ULL) | ((uint64_t)hwts_ns);
	if (hwts_ns > (hw_time & 0xffffffff)) /* the 32-bit clock counter wrapped between hwts_ns and now */
		nanoseconds -= 0x100000000ULL;

	return nanoseconds;
}

static int netc_1588_freerunning_convert(void *priv, uint64_t ns_src, struct hw_clock *clk_dst, uint64_t *ns_dst)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)priv;
	uint64_t t1, t2;
	int64_t dt;

	if (clk_dst != &hw_clock->clock[0])
		goto err;

	NETC_TimerGetFrtSrtTime(&hw_clock->timer_handle, &t1, &t2);

	dt = ns_src - t1;
	if ((dt < INT32_MIN) || (dt > INT32_MAX))
		goto err;

	dt = (dt * (1000000000LL + hw_clock->ppb)) / (hw_clock->clock[1].rate);

	*ns_dst = t2 + dt;

	return 0;

err:
	return -1;
}

static int netc_1588_convert(void *priv, uint64_t ns_src, struct hw_clock *clk_dst, uint64_t *ns_dst)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)priv;
	uint64_t t1, t2;
	int64_t dt;

	if (clk_dst != &hw_clock->clock[1])
		goto err;

	NETC_TimerGetFrtSrtTime(&hw_clock->timer_handle, &t1, &t2);

	dt = ns_src - t2;
	if ((dt < INT32_MIN) || (dt > INT32_MAX))
		goto err;

	dt = (dt * hw_clock->clock[1].rate) / (1000000000LL + hw_clock->ppb);

	*ns_dst = t1 + dt;

	return 0;

err:
	return -1;
}

static uint64_t netc_1588_freerunning_read_counter(void *priv)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)priv;
	uint64_t timestamp;

	NETC_TimerGetFreeRunningTime(&hw_clock->timer_handle, &timestamp);

	return timestamp;
}

static uint64_t netc_1588_read_counter(void *priv)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)priv;
	uint64_t timestamp;

	NETC_TimerGetCurrentTime(&hw_clock->timer_handle, &timestamp);

	return timestamp;
}

static int netc_1588_clock_adj_freq(void *priv, int32_t ppb)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)priv;

	hw_clock->ppb = ppb;

	NETC_TimerAdjustFreq(&hw_clock->timer_handle, ppb);

	return 0;
}

static int netc_1588_clock_set_offset(void *priv, int64_t offset)
{
	struct netc_hw_clock *hw_clock = (struct netc_hw_clock *)priv;

	NETC_TimerAddOffset(&hw_clock->timer_handle, offset);

	return 0;
}

static void netc_1588_timer_start_alarm(void *handle, uint8_t alarm_id, uint64_t nanosecond)
{
	NETC_TimerStartAlarm(handle, alarm_id, nanosecond);
}

static void netc_1588_timer_stop_alarm(void *handle, uint8_t alarm_id)
{
	NETC_TimerStopAlarm(handle, alarm_id);
}

static void netc_1588_timer_start_fiper(void *handle, uint8_t fiper_id, netc_timer_fiper_t *fiper)
{
	NETC_TimerStartFIPER(handle, fiper_id, fiper);
}

static void netc_1588_timer_stop_fiper(void *handle, uint8_t fiper_id)
{
	NETC_TimerStopFIPER(handle, fiper_id);
}

static int netc_1588_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct netc_1588_timer *netc_timer = container_of(hw_timer, struct netc_1588_timer, hw_timer);
	struct netc_hw_clock *hw_clock = container_of(netc_timer, struct netc_hw_clock, timer[netc_timer->index]);

	netc_1588_timer_start_alarm(&hw_clock->timer_handle, netc_timer->id, cycles);

	return 0;
}

static int netc_1588_timer_set_periodic_event(struct hw_timer *hw_timer, uint64_t offset, uint64_t period)
{
	struct netc_1588_timer *netc_timer = container_of(hw_timer, struct netc_1588_timer, hw_timer);
	struct netc_hw_clock *hw_clock = container_of(netc_timer, struct netc_hw_clock, timer[netc_timer->index]);
	netc_timer_fiper_t fiper = {.enableInterrupt = netc_timer->irq_enabled, .pulseGenSync = false, .pulseWidth = 30, .pulsePeriod = period};

	/* Verify if GENAVB_TIMERF_PPS flag is used */
	if (!hw_timer_is_pps_enabled(hw_timer) && netc_timer->is_pps)
		goto err;

	/* Ensure fiper period is uint32_t value */
	if ((period > 4 * hw_clock->gclk_period) && (period <= UINT32_MAX)) {
		fiper.pulsePeriod = period - hw_clock->tclk_period;
	} else {
		goto err;
	}

	/* Ensure pulse width is within limits */
	if ((fiper.pulseWidth * hw_clock->gclk_period) > (fiper.pulsePeriod / 2))
		fiper.pulseWidth = fiper.pulsePeriod / (2* hw_clock->gclk_period);

	/* Start fiper before Alarm1*/
	netc_1588_timer_start_fiper(&hw_clock->timer_handle, netc_timer_id[netc_timer->id], &fiper);

	netc_1588_timer_start_alarm(&hw_clock->timer_handle, kNETC_TimerAlarm1, offset); /* start Alarm1 */

	return 0;

err:
	return -1;
}

static int netc_1588_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct netc_1588_timer *netc_timer = container_of(hw_timer, struct netc_1588_timer, hw_timer);
	struct netc_hw_clock *hw_clock = container_of(netc_timer, struct netc_hw_clock, timer[netc_timer->index]);

	if (netc_timer->id >= NETC_TIMERID_FIPER1) {
		netc_1588_timer_stop_fiper(&hw_clock->timer_handle, netc_timer_id[netc_timer->id]);
	} else {
		netc_1588_timer_stop_alarm(&hw_clock->timer_handle, netc_timer_id[netc_timer->id]);
	}

	return 0;
}

void netc_timer_callback(void *data)
{
	/* check which timer generated the IRQ (TMR_EVENT register, AFAIK), map to the struct hw_timer and call .func */
	struct netc_hw_clock *netc_hw_clock = data;
	struct netc_1588_timer *timer;
	int i;

	/* Check if the event is set in the timer event register */
	for (i = 0; i < netc_hw_clock->timer_n; i++) {
		timer = &netc_hw_clock->timer[i];

		if (!timer->enabled)
			continue;

		if ((hw_clock->timer_handle.hw.base->TMR_TEVENT & timer->irq_flag) != 0U) {
			if (timer->hw_timer.func) {
				timer->hw_timer.func(timer->hw_timer.data);
			} else {
				os_log(LOG_ERR, "invalid isr, timer id: %d\n", timer->id);
			}

			hw_clock->timer_handle.hw.base->TMR_TEVENT = timer->irq_flag;
		}
	}
}

/**
 * \brief local function to deinitalize the Netc 1588 Timer
 *
 * \param id a clock id for local hw_clock array
 *
 */
void __netc_1588_exit(unsigned int id)
{
	struct netc_hw_clock *netc_hw_clock = &hw_clock[id];

	hw_clock_unregister(HW_CLOCK_PORT_BASE + netc_hw_clock->id);

	if (netc_hw_clock->owner) {
		hw_clock_unregister(HW_CLOCK_PORT_BASE + netc_hw_clock->id + 1);

		NETC_TimerDeinit(&netc_hw_clock->timer_handle);
	}
}

bool netc_1588_freerunning_available(unsigned int hw_clock_id)
{
	struct netc_hw_clock *netc_hw_clock;
	int i;

	for (i = 0; i < CFG_NUM_NETC_HW_CLOCK; i++) {
		netc_hw_clock = &hw_clock[i];

		if ((netc_hw_clock->id == hw_clock_id) && netc_hw_clock->owner)
			goto found;
	}

	return false;

found:
	return true;
}

/**
 * \brief local function to initalize the Netc 1588 Timer
 *
 * \param id a clock id for local hw_clock array
 * \return 0 on success and -1 on error
 */
int __netc_1588_init(unsigned int id)
{
	struct netc_hw_clock *netc_hw_clock = &hw_clock[id];
	struct netc_1588_timer *timer;
	struct hw_timer *hw_timer;
	netc_timer_alarm_t alarm = {.pulseGenSync = false, .pulseWidth = 0, .polarity = false, .enableInterrupt = true};
	netc_timer_fiper_config_t fiper = {.startCondition = true, .fiper1Loopback = false, .fiper2Loopback = false, .prescale = FIPER_PRESCALE};
	bool alarm1_used = false, fiper_used = false, irq_enabled = false;
	uint32_t refclk;
	int i;

	/* Run time constraints
	* - Timer IRQ's can be used by a single CPU (shared MSGINTR vector/instance)
	* - All Fipers need to be assigned to same CPU (shared TMR_FIPER_CTRL register)
	* - Timer configuration must be done in the same CPU (shared TMR_CTRL and TMR_TEMASK registers)
	* - Fipers are always started synchronously (using ALARM1)
	* - Only one Fiper can be started at a time (shared ALARM1)
	* - Freerunning clock can be accessed by a single CPU (side effect on TMR_SRT_H/L registers)
	*/

	for (i = 0; i < netc_hw_clock->timer_n; i++) {
		timer = &netc_hw_clock->timer[i];

		if (timer->id >= NETC_TIMERID_MAX)
			goto err;

		timer->index = i;

		timer->irq_flag = netc_timer_irq_flag[timer->id];

		if (timer->id == NETC_TIMERID_ALARM1)
			alarm1_used = true;

		if (timer->id >= NETC_TIMERID_FIPER1)
			fiper_used = true;

		if (timer->irq_enabled && timer->enabled)
			irq_enabled = true;
	}

	if (alarm1_used && fiper_used) {
		os_log(LOG_INFO, "Alarm1 is unavailable\n");
		goto err;
	}

	refclk = dev_get_net_1588_freq(SW0_BASE);

	netc_hw_clock->tclk_period = NSECS_PER_SEC / refclk;

	netc_hw_clock->gclk_period = ((uint64_t) fiper.prescale * NSECS_PER_SEC) / refclk;

	os_log(LOG_INFO, "timerClk period (ns): %u genClk period (ns): %u\n", netc_hw_clock->tclk_period, netc_hw_clock->gclk_period);

	if (netc_hw_clock->owner) {
		netc_timer_config_t timer_config;
		netc_msix_entry_t timerMsixEntry;

		/* MSIX configuration.*/
		timerMsixEntry.control = kNETC_MsixIntrMaskBit;
		timerMsixEntry.msgAddr = msgintr_msix_msgaddr(netc_hw_clock->msgintr.base, netc_hw_clock->msgintr.channel);
		timerMsixEntry.msgData = TIMER_MSG_DATA;

		memset(&timer_config, 0, sizeof(netc_timer_config_t));
		timer_config.msixEntry = &timerMsixEntry;
		timer_config.entryNum  = 1;
		timer_config.clockSelect = kNETC_TimerExtRefClk;
		timer_config.refClkHz = refclk;
		timer_config.enableTimer = true;

		os_log(LOG_INFO, "refClkHz: %u defaultPpb: %u\n", timer_config.refClkHz, timer_config.defaultPpb);

		if (NETC_TimerInit(&netc_hw_clock->timer_handle, &timer_config) != kStatus_Success)
			goto err;

		/* Unmask MSIX message interrupt. */
		NETC_TimerMsixSetEntryMask(&netc_hw_clock->timer_handle, 0, false);

		for (i = 0; i < netc_hw_clock->timer_n; i++) {
			timer = &netc_hw_clock->timer[i];

			switch (timer->id) {
			case NETC_TIMERID_ALARM1:
				alarm.enableInterrupt = timer->irq_enabled;
				NETC_TimerConfigureAlarm(&netc_hw_clock->timer_handle, netc_timer_id[timer->id], &alarm);

				os_log(LOG_INFO, "Alarm1 is configured\n");

				break;

			case NETC_TIMERID_ALARM2:
				alarm.enableInterrupt = timer->irq_enabled;
				NETC_TimerConfigureAlarm(&netc_hw_clock->timer_handle, netc_timer_id[timer->id], &alarm);

				os_log(LOG_INFO, "Alarm2 is configured\n");

				break;

			default:
				break;
			}
		}

		if (fiper_used) {
			NETC_TimerConfigureFIPER(&netc_hw_clock->timer_handle, &fiper);

			alarm.enableInterrupt = false;
			NETC_TimerConfigureAlarm(&netc_hw_clock->timer_handle, kNETC_TimerAlarm1, &alarm);

			os_log(LOG_INFO, "Alarm1 is configured for Fiper\n");
		}

		/* Enable Timer */
		NETC_TimerEnable(&netc_hw_clock->timer_handle, true);
	} else {
		os_log(LOG_INFO, "NETC 1588 Timer has started\n");
		NETC_TimerInitHandle(&netc_hw_clock->timer_handle);
	}

	if (irq_enabled) {
		if (msgintr_init_vector(netc_hw_clock->msgintr.base, netc_hw_clock->msgintr.channel, &netc_timer_callback, netc_hw_clock) == 0)
			goto err_msgintr_vector;
	}

	netc_hw_clock->clock[0].read_counter = netc_1588_read_counter;

	if (netc_hw_clock->owner) {
		netc_hw_clock->clock[0].set_offset = netc_1588_clock_set_offset;
		netc_hw_clock->clock[0].adj_freq = netc_1588_clock_adj_freq;
		netc_hw_clock->clock[0].convert = netc_1588_convert;
	}

	netc_hw_clock->clock[0].priv = netc_hw_clock;

	if (hw_clock_register(HW_CLOCK_PORT_BASE + netc_hw_clock->id, &netc_hw_clock->clock[0]) < 0) {
		os_log(LOG_ERR, "failed to register hw_clock(%d)\n", HW_CLOCK_PORT_BASE + netc_hw_clock->id);
		goto err_clock_register;
	}

	if (netc_hw_clock->owner) {
		netc_hw_clock->clock[1].rate = refclk;
		netc_hw_clock->clock[1].read_counter = netc_1588_freerunning_read_counter;
		netc_hw_clock->clock[1].convert = netc_1588_freerunning_convert;
		netc_hw_clock->clock[1].priv = netc_hw_clock;

		if (hw_clock_register(HW_CLOCK_PORT_BASE + netc_hw_clock->id + 1, &netc_hw_clock->clock[1]) < 0) {
			os_log(LOG_ERR, "failed to register hw_clock(%d)\n", HW_CLOCK_PORT_BASE + netc_hw_clock->id + 1);
			goto err_clock_register1;
		}
	}

	for (i = 0; i < netc_hw_clock->timer_n; i++) {
		unsigned int flags = 0;

		timer = &netc_hw_clock->timer[i];
		hw_timer = &timer->hw_timer;

		if (!timer->enabled)
			continue;

		if (timer->is_pps)
			flags |= HW_TIMER_F_PPS;

		if (timer->id >= NETC_TIMERID_FIPER1) {
			hw_timer->set_periodic_event = netc_1588_timer_set_periodic_event;
			flags |= HW_TIMER_F_PERIODIC;
		} else {
			hw_timer->set_next_event = netc_1588_timer_set_next_event;
		}

		hw_timer->cancel_event = netc_1588_timer_cancel_event;

		if (hw_timer_register(HW_CLOCK_PORT_BASE + netc_hw_clock->id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "failed to register hw timer\n");
			continue;
		}
	}

	return 0;

err_clock_register1:
	hw_clock_unregister(HW_CLOCK_PORT_BASE + netc_hw_clock->id);

err_clock_register:
	if (irq_enabled)
		msgintr_reset_vector(netc_hw_clock->msgintr.base, netc_hw_clock->msgintr.channel);

err_msgintr_vector:
	if (netc_hw_clock->owner) {
		NETC_TimerMsixSetEntryMask(&netc_hw_clock->timer_handle, 0, true); /* Mask MSI-X */
		NETC_TimerDeinit(&netc_hw_clock->timer_handle);
	}
err:
	return -1;
}

void netc_1588_exit(void)
{
	int i;

	for (i = 0; i < CFG_NUM_NETC_HW_CLOCK; i++)
		__netc_1588_exit(i);
}

int netc_1588_init(void)
{
	int i;

	for (i = 0; i < CFG_NUM_NETC_HW_CLOCK; i++)
		if (__netc_1588_init(i) < 0)
			goto err;

	return 0;

err:
#if CFG_NUM_NETC_HW_CLOCK > 1
	for (i--; i >= 0; i--)
		__netc_1588_exit(i);
#endif /* if CFG_NUM_NETC_HW_CLOCK > 1 */

	return -1;
}

#else
int netc_1588_init(void) { return 0; }
void netc_1588_exit(void) {}
uint64_t netc_1588_hwts_to_u64(struct hw_clock *clk, uint32_t hwts_ns) { return 0; }
#endif /* CFG_NUM_NETC_HW_CLOCK */
