/*
 * Copyright 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB GPT driver
 @details
*/

#include "common/log.h"

#include "config.h"
#include "gpt.h"
#include "gpt_rec.h"
#include "hw_timer.h"
#include "net_port.h"

#if CFG_NUM_GPT > 0

#include "fsl_gpt.h"

#define GPT_NUM_TIMER		3

struct gpt_output_compare {
	gpt_output_compare_channel_t channel;
	gpt_interrupt_enable_t interrupt_mask;
	gpt_status_flag_t status_flag_mask;
};

struct gpt_timer {
	unsigned int id;
	struct hw_timer hw_timer;
	struct gpt_output_compare compare;
};

struct gpt_dev {
	GPT_Type *base;
	IRQn_Type irq;

	clock_name_t input_clock;
	uint32_t gpt_input_clk_rate;
	gpt_clock_source_t clk_src_type;
	uint32_t prescale;

	struct gpt_timer timer[GPT_NUM_TIMER];

	struct hw_clock clock;
	hw_clock_id_t clock_id;
};

static struct gpt_dev gpt_devices[CFG_NUM_GPT] = {
	[0] = {
		.base = BOARD_GPT_0_BASE,
		.irq = BOARD_GPT_0_IRQ,

#ifdef BOARD_GPT_0_PRESCALE
		.prescale = BOARD_GPT_0_PRESCALE,
#else
		.prescale = 1,
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
		.clock_id = HW_CLOCK_MONOTONIC,
		.timer = {
			[0] = {
				.id = 0,
				.compare = {
					.channel = kGPT_OutputCompare_Channel1,
					.interrupt_mask = kGPT_OutputCompare1InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare1Flag,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kGPT_OutputCompare_Channel2,
					.interrupt_mask = kGPT_OutputCompare2InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare2Flag,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kGPT_OutputCompare_Channel3,
					.interrupt_mask = kGPT_OutputCompare3InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare3Flag,
				},
			},
		},
	},
#if CFG_NUM_GPT > 1
	[1] = {
		.base = BOARD_GPT_1_BASE,
		.irq = BOARD_GPT_1_IRQ,
#ifdef BOARD_GPT_1_PRESCALE
		.prescale = BOARD_GPT_1_PRESCALE,
#else
		.prescale = 1,
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
					.channel = kGPT_OutputCompare_Channel1,
					.interrupt_mask = kGPT_OutputCompare1InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare1Flag,
				},
			},
			[1] = {
				.id = 1,
				.compare = {
					.channel = kGPT_OutputCompare_Channel2,
					.interrupt_mask = kGPT_OutputCompare2InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare2Flag,
				},
			},
			[2] = {
				.id = 2,
				.compare = {
					.channel = kGPT_OutputCompare_Channel3,
					.interrupt_mask = kGPT_OutputCompare3InterruptEnable,
					.status_flag_mask = kGPT_OutputCompare3Flag,
				},
			},
		},
	},
#endif
#if CFG_NUM_GPT > 2
#error invalid CFG_NUM_GPT
#endif
};

static void gpt_timer_interrupt(struct gpt_dev *dev, uint32_t flags)
{
	struct gpt_timer *timer = &dev->timer[0];

	while (flags) {
		if (flags & 1) {
			GPT_DisableInterrupts(dev->base,
					      timer->compare.interrupt_mask);

			GPT_ClearStatusFlags(dev->base,
					     timer->compare.status_flag_mask);

			if (timer->hw_timer.func)
				timer->hw_timer.func(timer->hw_timer.data);
		}

		flags >>= 1;
		timer++;
	};
}

static void gpt_dev_interrupt(struct gpt_dev *dev)
{
	uint32_t flags = GPT_GetStatusFlags(dev->base,
					    dev->base->IR & (kGPT_OutputCompare1InterruptEnable |
							     kGPT_OutputCompare2InterruptEnable |
							     kGPT_OutputCompare3InterruptEnable));

	gpt_timer_interrupt(dev, flags);
}

void BOARD_GPT_0_IRQ_HANDLER(void)
{
	gpt_dev_interrupt(&gpt_devices[0]);

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
	  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif
}

#if CFG_NUM_GPT > 1
void BOARD_GPT_1_IRQ_HANDLER(void)
{
	gpt_dev_interrupt(&gpt_devices[1]);

#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
	__DSB();
#endif
}
#endif

static int gpt_hw_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct gpt_timer *gpt_timer = container_of(hw_timer, struct gpt_timer, hw_timer);
	struct gpt_dev *gpt_dev = container_of(gpt_timer, struct gpt_dev, timer[gpt_timer->id]);

	GPT_ClearStatusFlags(gpt_dev->base, gpt_timer->compare.status_flag_mask);
	GPT_EnableInterrupts(gpt_dev->base, gpt_timer->compare.interrupt_mask);
	GPT_SetOutputCompareValue(gpt_dev->base, gpt_timer->compare.channel, cycles);

	return 0;
}

static int gpt_hw_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct gpt_timer *gpt_timer = container_of(hw_timer, struct gpt_timer, hw_timer);
	struct gpt_dev *gpt_dev = container_of(gpt_timer, struct gpt_dev, timer[gpt_timer->id]);

	GPT_DisableInterrupts(gpt_dev->base, gpt_timer->compare.interrupt_mask);

	return 0;
}

static uint64_t gpt_read_counter(void *priv)
{
	struct gpt_dev *gpt_dev = (struct gpt_dev *)priv;

	return GPT_GetCurrentTimerCount(gpt_dev->base);
}

__init static void gpt_reg_init(struct gpt_dev *dev)
{
	gpt_config_t cfg;

	GPT_GetDefaultConfig(&cfg);

	cfg.enableFreeRun = true;
	cfg.enableRunInWait = true;
	cfg.clockSource = dev->clk_src_type;
	cfg.divider = dev->prescale;

	GPT_Init(dev->base, &cfg);

	GPT_StartTimer(dev->base);
}

__init static int gpt_init(struct gpt_dev *dev)
{
	struct hw_clock *clock = &dev->clock;
	unsigned int flags = 0;
	int i;

	dev->clk_src_type = BOARD_GPT_clk_src(dev->base);
	dev->gpt_input_clk_rate = BOARD_GPT_clk_freq(dev->base);

	gpt_reg_init(dev);

	EnableIRQ(dev->irq);

	/* HW Clock */
	clock->rate = dev->gpt_input_clk_rate / dev->prescale;
	clock->read_counter = gpt_read_counter;
	clock->priv = dev;

	if (hw_clock_register(dev->clock_id, clock) < 0)
		os_log(LOG_ERR, "failed to register hw_clock(%d)\n", dev->clock_id);

#ifdef BOARD_GPT_REC_BASE
	if (dev->base == BOARD_GPT_REC_BASE)
		flags |= HW_TIMER_F_RECOVERY;
#endif

	/* HW Timers */
	for (i = 0; i < GPT_NUM_TIMER; i++) {
		struct hw_timer *hw_timer = &dev->timer[i].hw_timer;

#ifdef BOARD_GPT_REC_BASE
		if ((dev->base == BOARD_GPT_REC_BASE) && (i == (BOARD_GPT_REC_CHANNEL - 1)))
			continue;
#endif
		hw_timer->set_next_event = gpt_hw_timer_set_next_event;
		hw_timer->cancel_event = gpt_hw_timer_cancel_event;

		if (hw_timer_register(dev->clock_id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "%s : failed to register hw timer\n", __func__);
			continue;
		}
	}

	return 0;
}

__exit static int gpt_exit(struct gpt_dev *dev)
{
	DisableIRQ(dev->irq);
	GPT_Deinit(dev->base);

	hw_clock_unregister(dev->clock_id);

	return 0;
}

__init int gpt_driver_init(void)
{
	int i, rc = 0;

	for (i = 0; i < CFG_NUM_GPT; i++) {
		if (gpt_init(&gpt_devices[i]) < 0) {
			os_log(LOG_ERR, "gpt_init failed: device %d\n", i);
			rc = -1;
			goto err;
		}
	}

	if (gpt_rec_init() < 0)
		os_log(LOG_ERR, "failed to register GPT media clock recovery\n");

	return rc;
err:
#if CFG_NUM_GPT > 1
	for (i--; i >= 0; i--)
		gpt_exit(&gpt_devices[i]);
#endif

	return rc;
}

__exit void gpt_driver_exit(void)
{
	int i;

	gpt_rec_exit();

	for (i = 0; i < CFG_NUM_GPT; i++)
		gpt_exit(&gpt_devices[i]);
}
#else
__init int gpt_driver_init(void) { return 0; }
__exit void gpt_driver_exit(void) { }
#endif /* CFG_NUM_GPT */
