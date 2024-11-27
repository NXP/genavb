/*
* Copyright 2019-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
  @file		management.c
  @brief	Management module implementation.
  @details
*/


#include "os/stdlib.h"

#include "common/log.h"
#include "common/ether.h"

#include "management.h"

__init static struct management_ctx *management_alloc(unsigned int ports, unsigned int timers)
{
	struct management_ctx *management;
	unsigned int size;

	size = sizeof(struct management_ctx) + timer_pool_size(timers);
	size += sizeof(struct mac_service) + ports * sizeof(struct mac_port);

	management = os_malloc(size);
	if (!management)
		goto err;

	os_memset(management, 0, size);

	management->timer_ctx = (struct timer_ctx *)(management + 1);
	management->mac = (struct mac_service *)((u8 *)management->timer_ctx + timer_pool_size(timers));

	management->mac->management = management;

	return management;

err:
	return NULL;
}

/** Management code entry points.
 * \return	0 on success or negative value on failure
 * \param management	pointer to the main management context
 */
__init void *management_init(struct management_config *cfg, unsigned long priv)
{
	struct management_ctx *management;
	unsigned int timer_n;

	timer_n = CFG_MANAGEMENT_MAX_TIMERS;

	management = management_alloc(cfg->port_max, timer_n);
	if (!management)
		goto err_malloc;

	log_level_set(management_COMPONENT_ID, cfg->log_level);

	if (timer_pool_init(management->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	if (mac_service_init(management->mac, cfg, priv) < 0)
		goto err_mac_service;

	os_log(LOG_INIT, "management(%p) done\n", management);

	return management;

err_mac_service:
	timer_pool_exit(management->timer_ctx);

err_timer_pool_init:
	os_free(management);

err_malloc:
	return NULL;
}


/** Management exit function. Terminate and clean-up management sub-components
 * \return	0 on success or negative value on failure
 * \param management	pointer to the main management context
 */
__exit int management_exit(void *management_ctx)
{
	struct management_ctx *management = (struct management_ctx *)management_ctx;

	mac_service_exit(management->mac);

	timer_pool_exit(management->timer_ctx);

	os_log(LOG_INIT, "management(%p)\n", management);

	os_free(management);

	return 0;
}
