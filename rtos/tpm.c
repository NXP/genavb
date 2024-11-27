/*
	tpm_driver_exit();
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief AVB TPM driver
 @details
*/

#include "common/log.h"

#include "config.h"
#include "tpm.h"
#include "tpm_rec.h"
#include "hw_timer.h"
#include "net_port.h"

#if CFG_NUM_TPM > 0

#include "fsl_tpm.h"

#define TPM_NUM_TIMER		4 /* Based on number of available channels on i.MX 93 */

struct tpm_output_compare {
	tpm_chnl_t channel;
	tpm_interrupt_enable_t interrupt_mask;
	tpm_status_flags_t status_flag_mask;
};

struct tpm_timer {
	unsigned int id;
	struct hw_timer hw_timer;
	struct tpm_output_compare compare;
};

struct tpm_dev {
	TPM_Type *base;
	IRQn_Type irq;

	clock_name_t input_clock;
	uint32_t tpm_input_clk_rate;
	tpm_clock_source_t clk_src_type;
	tpm_clock_prescale_t prescale;
	uint32_t timerChannels; /* Channels used as timer: logical OR of members of tpm_status_flags_t */

	struct tpm_timer timer[TPM_NUM_TIMER];

	struct hw_clock clock;
	hw_clock_id_t clock_id;
};

static struct tpm_dev tpm_devices[CFG_NUM_TPM] = {
	[0] = {
		.base = BOARD_TPM_0_BASE,
		.irq = BOARD_TPM_0_IRQ,

#ifdef BOARD_TPM_0_PRESCALE
		.prescale = BOARD_TPM_0_PRESCALE,
#else
		.prescale = kTPM_Prescale_Divide_1,
#endif
		.clock = {
			/* period is initialized at run time */
			.to_ns = {
				.shift = 24,
			},
			.to_cyc = {
				.shift = 32,
			},
		},
		.clock_id = HW_CLOCK_MONOTONIC,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kTPM_Chnl_0,
					.interrupt_mask = kTPM_Chnl0InterruptEnable,
					.status_flag_mask = kTPM_Chnl0Flag,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kTPM_Chnl_1,
					.interrupt_mask = kTPM_Chnl1InterruptEnable,
					.status_flag_mask = kTPM_Chnl1Flag,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kTPM_Chnl_2,
					.interrupt_mask = kTPM_Chnl2InterruptEnable,
					.status_flag_mask = kTPM_Chnl2Flag,
				},
			},
			[3] = {
				.id = 3,
				.compare = {
					.channel = kTPM_Chnl_3,
					.interrupt_mask = kTPM_Chnl3InterruptEnable,
					.status_flag_mask = kTPM_Chnl3Flag,
				},
			},
		},
	},
#if CFG_NUM_TPM > 1
	[1] = {
		.base = BOARD_TPM_1_BASE,
		.irq = BOARD_TPM_1_IRQ,
#ifdef BOARD_TPM_1_PRESCALE
		.prescale = BOARD_TPM_1_PRESCALE,
#else
		.prescale = kTPM_Prescale_Divide_1,
#endif

		.clock = {
			.period = ((uint64_t)1 << 32),
			.to_ns = {
				.shift = 24,
			},
			.to_cyc = {
				.shift = 32,
			},
		},
		.clock_id = HW_CLOCK_MONOTONIC_1,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kTPM_Chnl_0,
					.interrupt_mask = kTPM_Chnl0InterruptEnable,
					.status_flag_mask = kTPM_Chnl0Flag,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kTPM_Chnl_1,
					.interrupt_mask = kTPM_Chnl1InterruptEnable,
					.status_flag_mask = kTPM_Chnl1Flag,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kTPM_Chnl_2,
					.interrupt_mask = kTPM_Chnl2InterruptEnable,
					.status_flag_mask = kTPM_Chnl2Flag,
				},
			},
			[3] = {
				.id = 3,
				.compare = {
					.channel = kTPM_Chnl_3,
					.interrupt_mask = kTPM_Chnl3InterruptEnable,
					.status_flag_mask = kTPM_Chnl3Flag,
				},
			},
		},
	},
#endif
#if CFG_NUM_TPM > 2
#error invalid CFG_NUM_TPM
#endif
};

static void tpm_timer_interrupt(struct tpm_dev *dev, uint32_t flags)
{
	struct tpm_timer *timer = &dev->timer[0];

	while (flags) {
		if (flags & 1) {
			TPM_DisableInterrupts(dev->base,
					      timer->compare.interrupt_mask);

			TPM_ClearStatusFlags(dev->base,
					     timer->compare.status_flag_mask);

			if (timer->hw_timer.func)
				timer->hw_timer.func(timer->hw_timer.data);
		}

		flags >>= 1;
		timer++;
	};
}

static void tpm_dev_interrupt(struct tpm_dev *dev)
{
	/* Handle only timer interrupts (some channels can be used for recovery for example). */
	uint32_t enabledInterrupts = TPM_GetEnabledInterrupts(dev->base) & dev->timerChannels;
	uint32_t flags = TPM_GetStatusFlags(dev->base) & enabledInterrupts;

	tpm_timer_interrupt(dev, flags);
}

void BOARD_TPM_0_IRQ_HANDLER(void)
{
	tpm_dev_interrupt(&tpm_devices[0]);

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
	  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif
}

#if CFG_NUM_TPM > 1
void BOARD_TPM_1_IRQ_HANDLER(void)
{
	tpm_dev_interrupt(&tpm_devices[1]);

#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif
}
#endif

static int tpm_hw_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct tpm_timer *tpm_timer = container_of(hw_timer, struct tpm_timer, hw_timer);
	struct tpm_dev *tpm_dev = container_of(tpm_timer, struct tpm_dev, timer[tpm_timer->id]);

	TPM_ClearStatusFlags(tpm_dev->base, tpm_timer->compare.status_flag_mask);
	TPM_EnableInterrupts(tpm_dev->base, tpm_timer->compare.interrupt_mask);
	TPM_SetupOutputCompare(tpm_dev->base, tpm_timer->compare.channel, kTPM_NoOutputSignal, cycles);

	return 0;
}

static int tpm_hw_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct tpm_timer *tpm_timer = container_of(hw_timer, struct tpm_timer, hw_timer);
	struct tpm_dev *tpm_dev = container_of(tpm_timer, struct tpm_dev, timer[tpm_timer->id]);

	TPM_DisableInterrupts(tpm_dev->base, tpm_timer->compare.interrupt_mask);

	return 0;
}

static uint64_t tpm_read_counter(void *priv)
{
	struct tpm_dev *tpm_dev = (struct tpm_dev *)priv;

	return TPM_GetCurrentTimerCount(tpm_dev->base);
}

__init static void tpm_reg_init(struct tpm_dev *dev)
{
	tpm_config_t tpmConfig;

	TPM_GetDefaultConfig(&tpmConfig);
	tpmConfig.prescale = dev->prescale;
	TPM_Init(dev->base, &tpmConfig);

	/* Set the modulo to max value. */
	dev->base->MOD = TPM_MAX_COUNTER_VALUE(base);

	TPM_StartTimer(dev->base, dev->clk_src_type);
}

__init static int tpm_init(struct tpm_dev *dev)
{
	struct hw_clock *clock = &dev->clock;
	unsigned int flags = 0;
	int i;

	dev->clk_src_type = BOARD_TPM_clk_src(dev->base);
	dev->tpm_input_clk_rate = BOARD_TPM_clk_freq(dev->base);

	/* Some (old) variants of TPM has only support for 16 bits counters, which can cause issues
	 * for the core timer code (does not expect such small periods.
	 * Avoid registering such devices and report an error for now.)
	 */
	if ((uint8_t)FSL_FEATURE_TPM_HAS_32BIT_COUNTERn(dev->base) != 1) {
		os_log(LOG_ERR, "failed to init TPM device(%p): does not support 32 bits\n", dev->base);
		return -1;
	}

	tpm_reg_init(dev);

	EnableIRQ(dev->irq);

	/* HW Clock */
	clock->rate = dev->tpm_input_clk_rate / (1 << dev->prescale);
	clock->read_counter = tpm_read_counter;
	clock->priv = dev;

	/* Set the period based on maximum supported max value. */
	clock->period = (uint64_t)TPM_MAX_COUNTER_VALUE(dev->base) + 1;

	if (hw_clock_register(dev->clock_id, clock) < 0)
		os_log(LOG_ERR, "failed to register hw_clock(%d)\n", dev->clock_id);

#ifdef BOARD_TPM_REC_BASE
	if (dev->base == BOARD_TPM_REC_BASE)
		flags |= HW_TIMER_F_RECOVERY;
#endif

	dev->timerChannels = 0;

	/* HW Timers */
	for (i = 0; i < TPM_NUM_TIMER; i++) {
		struct hw_timer *hw_timer = &dev->timer[i].hw_timer;

		/* TPM channels can either be output compare or input capture, skip
		 * registering the channel used for recovery.
		 */
#ifdef BOARD_TPM_REC_BASE
		if ((dev->base == BOARD_TPM_REC_BASE) && (i == BOARD_TPM_REC_CHANNEL))
			continue;
#endif
		hw_timer->set_next_event = tpm_hw_timer_set_next_event;
		hw_timer->cancel_event = tpm_hw_timer_cancel_event;

		if (hw_timer_register(dev->clock_id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "%s : failed to register hw timer\n", __func__);
			continue;
		}

		dev->timerChannels |= dev->timer[i].compare.status_flag_mask;
	}

	return 0;
}

__exit static int tpm_exit(struct tpm_dev *dev)
{
	DisableIRQ(dev->irq);
	TPM_Deinit(dev->base);

	hw_clock_unregister(dev->clock_id);

	return 0;
}

__init int tpm_driver_init(void)
{
	int i, rc = 0;

	for (i = 0; i < CFG_NUM_TPM; i++) {
		if (tpm_init(&tpm_devices[i]) < 0) {
			os_log(LOG_ERR, "tpm_init failed: device %d\n", i);
			rc = -1;
			goto err;
		}
	}

	if (tpm_rec_init() < 0)
		os_log(LOG_ERR, "failed to register TPM media clock recovery\n");

	return rc;
err:
#if CFG_NUM_TPM > 1
	for (i--; i >= 0; i--)
		tpm_exit(&tpm_devices[i]);
#endif

	return rc;
}

__exit void tpm_driver_exit(void)
{
	int i;

	tpm_rec_exit();

	for (i = 0; i < CFG_NUM_TPM; i++)
		tpm_exit(&tpm_devices[i]);
}
#else
__init int tpm_driver_init(void) { return 0; }
__exit void tpm_driver_exit(void) { }
#endif /* CFG_NUM_TPM */
