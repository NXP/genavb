/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file msgintr.c
 @brief RTOS specific Message Interrupt block implementation
 @details
*/

#include "common/log.h"
#include "types.h"
#include "config.h"

#include "msgintr.h"

/* Giving basic functionality to the cores M33 and M7 in rt1180 */
#if CFG_NUM_MSGINTR >= 0

#include "fsl_msgintr.h"

uint32_t msgintr_msix_msgaddr(void *base, uint8_t channel)
{
	os_log(LOG_INFO, "msgintr addr return for the MSI-X Entry\n");
	return MSGINTR_GetIntrSelectAddr((MSGINTR_Type *) base, channel);
}

/* Full msgintr support for core provided when CFG_NUM_MSGINTR is 1 */
#if CFG_NUM_MSGINTR > 0

struct msgintr {
	MSGINTR_Type *base; /* MSGINT base address */
	struct {
		bool enabled; /* Enable channel */
		void (*cb)(void *data); /* Callback for interrupt per channel */
		void *data; /* Interrupt data from the callback */
	} channels[FSL_MSGINTR_CHANNEL_NUM];
};

static struct msgintr msgintr[CFG_NUM_MSGINTR] = {
	[0] = {
		.base = BOARD_MSGINTR0_BASE,
		.channels = {
			[0] = {
				.enabled = false,
			},
			[1] = {
				.enabled = false,
			},
			[2] = {
				.enabled = false,
			}
		},

#if CFG_NUM_MSGINTR > 1
#error `CFG_NUM_MSGINTR` more than 1 is not supported
#endif

	},
};

uint32_t msgintr_init_vector(void *base, uint8_t channel, void (*callback)(void *data), void *data)
{
	MSGINTR_Type *msgintr_base = (MSGINTR_Type *) base;
	int index;
	uint32_t msgAddr;

	os_log(LOG_INFO, "msgintr init vector configured\n");

	for (index = 0; index < CFG_NUM_MSGINTR; index++) {
		if (msgintr[index].base == msgintr_base && msgintr[index].channels[channel].enabled == false) {
			msgAddr				= MSGINTR_GetIntrSelectAddr(msgintr[index].base, channel);
			msgintr[index].channels[channel].cb = callback;
			msgintr[index].channels[channel].data = data;
			msgintr[index].channels[channel].enabled = true;

			return msgAddr;
		}
	}

	return 0;
}

void msgintr_reset_vector(void *base, uint8_t channel)
{
	MSGINTR_Type *msgintr_base = (MSGINTR_Type *) base;
	int index;

	for (index = 0; index < CFG_NUM_MSGINTR; index++) {
		if (msgintr[index].base == msgintr_base) {
			if (msgintr[index].channels[channel].enabled == true) {
				msgintr[index].channels[channel].cb = NULL;
				msgintr[index].channels[channel].data = NULL;
				msgintr[index].channels[channel].enabled = false;
			}
		}
	}
}

static void msgintrCallback_0(MSGINTR_Type *base, uint8_t channel, uint32_t pendingIntr)
{
	/* Interrupt */
	if (pendingIntr != 0U) {
		if (msgintr[0].channels[channel].enabled == true) {
			msgintr[0].channels[channel].cb(msgintr[0].channels[channel].data);
		}
	}

}

/* This function assigns the callback for msgintr instance based on index */
static void *msgintrCallback(uint8_t callback_id)
{
	if (callback_id == 0) {
		return &msgintrCallback_0;
	}

	return NULL;
}

void msgintr_exit(void)
{
	int index;

	for (index = 0; index < CFG_NUM_MSGINTR; index++) {
		MSGINTR_Deinit(msgintr[index].base);
	}
}

int msgintr_init(void)
{
	int index;

	os_log(LOG_INFO, "msgintr initalized\n");

	for (index = 0 ; index < CFG_NUM_MSGINTR; index++) {
		if (MSGINTR_Init(msgintr[index].base, msgintrCallback(index)) != kStatus_Success) {
			goto err;
		}
	}

	return 0;

err:

#if CFG_NUM_MSGINTR > 1
	for (index--; index >= 0; index--)
		MSGINTR_Deinit(msgintr[index].base);
#endif /* if CFG_NUM_MSGINTR > 1 */

	return -1;
}
#else /* if CFG_NUM_MSGINTR is equal to 0 */
int msgintr_init(void) { return 0; }
void msgintr_exit(void) {}
uint32_t msgintr_init_vector(void *base, uint8_t channel, void (*callback)(void *data), void *data) { return 0; }
void msgintr_reset_vector(void *base, uint8_t channel) { }

#endif /* CFG_NUM_MSGINTR */

#else /* if CFG_NUM_MSGINTR is < 0 or no msgintr support */
int msgintr_init(void) { return 0; }
void msgintr_exit(void) {}
uint32_t msgintr_init_vector(void *base, uint8_t channel, void (*callback)(void *data), void *data) { return 0; }
void msgintr_reset_vector(void *base, uint8_t channel) { }
uint32_t msgintr_msix_msgaddr(void *base, uint8_t channel) { return 0; }

#endif /* CFG_NUM_MSGINTR */