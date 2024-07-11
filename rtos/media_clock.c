/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media clock interface handling
*/

#include "rtos_abstraction_layer.h"
#include "avtp.h"
#include "common/log.h"
#include "os/media_clock.h"
#include "media_clock_drv.h"

static struct mclock_drv mclk_drv;
struct mclock_drv *mclk_drv_h = NULL;
static struct mclock_ctx mclock_ctx;

int mclock_drift_adapt(struct mclock_dev *dev, unsigned int clk_media)
{
	int rc = 0;

	if (avtp_after_eq(dev->clk_timer, dev->next_drift)) {
		signed int err = clk_media - dev->clk_timer;

		/* Media clock is faster */
		if (err >= (int)dev->timer_period) {
			if (err > 3 * (int)dev->timer_period)
				rc = -1;

			dev->clk_timer += dev->timer_period;
		} else if (err <= -((int)dev->timer_period)) {
			if (err < -3 * (int)dev->timer_period)
				rc = -1;

			dev->clk_timer -= dev->timer_period;
		}

		dev->next_drift = dev->clk_timer + dev->drift_period;
	}

	return rc;
}

void mclock_set_ts_freq(struct mclock_dev *dev, unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	dev->ts_freq_p = ts_freq_p;
	dev->ts_freq_q = ts_freq_q;
	rational_init(&dev->ts_period, (unsigned long long)ts_freq_q * NSECS_PER_SEC, ts_freq_p);
}

void mclock_wake_up_init(struct mclock_dev *dev, unsigned int now)
{
	struct mtimer_dev *mtimer_dev;
	struct slist_node *entry;

	for (entry = slist_first(&dev->mtimer_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		mtimer_dev = container_of(entry, struct mtimer_dev, list);
		mtimer_wake_up_init(mtimer_dev, now);
	}
}

void mclock_wake_up_init_now(struct mclock_dev *dev, unsigned int now)
{
	struct mtimer_dev *mtimer_dev;
	struct slist_node *entry;

	for (entry = slist_first(&dev->mtimer_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		mtimer_dev = container_of(entry, struct mtimer_dev, list);
		mtimer_wake_up_init_now(mtimer_dev, now);
	}
}

int mclock_wake_up_configure(struct mclock_dev *dev, unsigned int wake_freq_p, unsigned int wake_freq_q)
{
	// TODO
	return 0;
}

void mclock_wake_up(struct mclock_dev *dev, unsigned int now)
{
	struct mtimer_dev *mtimer_dev;
	struct slist_node *entry;

	if (!(dev->flags & MCLOCK_FLAGS_INIT)) {
		for (entry = slist_first(&dev->mtimer_devices); !slist_is_last(entry); entry = slist_next(entry)) {
			mtimer_dev = container_of(entry, struct mtimer_dev, list);
			mtimer_wake_up(mtimer_dev, now);
		}
	}

}

int mclock_register_timer(struct mclock_dev *dev, int (*irq_func)(struct mclock_dev *, void *, unsigned int), unsigned int period)
{
	int i, rc = -1;
	struct mclock_drv *drv = dev->drv;

	rtos_mutex_lock(&drv->mutex, RTOS_WAIT_FOREVER);

	for (i = 0; i < MCLOCK_TIMER_MAX; i++) {
		if (drv->isr[i].flags & MCLOCK_TIMER_ACTIVE) {
			continue;
		} else {
			drv->isr[i].irq_func = irq_func;
			drv->isr[i].dev = dev;
			drv->isr[i].flags |= MCLOCK_TIMER_ACTIVE;
			drv->isr[i].period = period;
			drv->isr[i].count = period;
			dev->id = i;
			rc = 0;
			goto exit;
		}
	}

exit:
	rtos_mutex_unlock(&drv->mutex);

	return rc;
}

void mclock_unregister_timer(struct mclock_dev *dev)
{
	struct mclock_drv *drv = dev->drv;

	rtos_mutex_lock(&drv->mutex, RTOS_WAIT_FOREVER);

	drv->isr[dev->id].flags &= (~MCLOCK_TIMER_ACTIVE);

	rtos_mutex_unlock(&drv->mutex);
}

void mclock_interrupt(unsigned int hw_ticks, bool *wake)
{
	int i;
	struct mclock_timer *timer;
	struct mclock_timer_event e;
	unsigned int hw_ticks_now;

	for (i = 0; i < MCLOCK_TIMER_MAX; i++) {
		timer = &mclk_drv_h->isr[i];

		if (!(timer->flags & MCLOCK_TIMER_ACTIVE))
			continue;

		timer->ticks = 0;
		hw_ticks_now = hw_ticks;

		/* Determine how many timer ticks have elapsed, based on hw_ticks and timer period */
		while (hw_ticks_now--) {
			if (!(--timer->count)) {
				timer->ticks++;
				timer->count = timer->period;
			}
		}

		if (timer->ticks) {
			bool timer_wake = false;
			e.timer = timer;

			if (rtos_mqueue_send_from_isr(&mclock_ctx.queue, &e, RTOS_NO_WAIT, &timer_wake) < 0) {
				timer->event_err++;
				return;
			}

			if (timer_wake)
				*wake = true;
		}
	}
}

void mclock_register_device(struct mclock_dev *dev)
{
	struct mclock_drv *drv = mclk_drv_h;

	rtos_mutex_lock(&drv->mutex, RTOS_WAIT_FOREVER);

	slist_add_head(&mclk_drv_h->mclock_devices, &dev->list_node);

	dev->drv = drv;
	dev->flags = MCLOCK_FLAGS_FREE | MCLOCK_FLAGS_REGISTERED;

	rtos_mutex_unlock(&drv->mutex);

	os_log(LOG_INIT, "dev(%p)\n", dev);
}

int mclock_unregister_device(struct mclock_dev *dev)
{
	struct mclock_drv *drv = mclk_drv_h;
	struct slist_head *head = &drv->mclock_devices;
	int rc = -1;

	rtos_mutex_lock(&drv->mutex, RTOS_WAIT_FOREVER);

	if (dev->flags & MCLOCK_FLAGS_REGISTERED) {
		slist_del(head, &dev->list_node);
		dev->flags = 0;
		rc = 0;
	}

	rtos_mutex_unlock(&drv->mutex);

	return rc;
}

static void mclock_task(void *pvParameters)
{
	struct mclock_ctx *mclock = pvParameters;
	struct mclock_timer *timer;
	struct mclock_timer_event e;
	struct mclock_dev *dev;
	uint32_t ptp_now[CFG_PORTS];
	int i;

	while (1) {
		if (rtos_mqueue_receive(&mclock->queue, &e, RTOS_WAIT_FOREVER) < 0)
			continue;

		timer = e.timer;
		dev = timer->dev;

		for (i = 0; i < CFG_PORTS; i++)
			if (os_clock_gettime32(ports[i].clock_gptp[0], &ptp_now[i]) < 0)
				os_log(LOG_ERR, "os_clock_gettime32() failed\n");

		timer->irq_func(dev, ptp_now, timer->ticks);

		if (dev->flags & MCLOCK_FLAGS_WAKE_UP)
			mclock_wake_up(dev, dev->clk_timer);
	}

	os_log(LOG_INIT, "mclock(%p) task exited\n", mclock);

	rtos_thread_abort(NULL);
}

__init int mclock_init(void)
{
	struct mclock_drv *drv;
	int rc = 0;
	int i, j;

	memset(&mclk_drv, 0, sizeof(mclk_drv));

	mclk_drv_h = &mclk_drv;
	drv = mclk_drv_h;

	if (rtos_mutex_init(&drv->mutex) < 0) {
		os_log(LOG_ERR, "rtos_mutex_init failed\n");
		rc = -1;
		goto err;
	}

	for (i = 0; i < MCLOCK_DOMAIN_PTP_RANGE; i++) {
		rc = mclock_gen_ptp_init(&drv->gen_ptp[i]);
		if (rc < 0) {
			os_log(LOG_ERR, "mclock_gen_ptp_init() failed for PTP domain %d\n", i);
			goto err_ptp;
		}
	}

	if (rtos_mqueue_init(&mclock_ctx.queue, MCLOCK_EVENT_QUEUE_LENGTH, sizeof(struct mclock_timer_event), mclock_ctx.queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init(mclock_ctx) failed\n");
		rc = -1;
		goto err_queue;
	}


	if (rtos_thread_create(&mclock_ctx.task, MCLOCK_TASK_PRIORITY, 0, MCLOCK_TASK_STACK_DEPTH, MCLOCK_TASK_NAME, mclock_task, &mclock_ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", MCLOCK_TASK_NAME);
		rc = -1;
		goto err_task_create;
	}

	return 0;

err_task_create:
err_queue:
	for (j = 0; j < i; j++)
		mclock_gen_ptp_exit(&drv->gen_ptp[j]);
err_ptp:
err:
	return rc;
}

__exit void mclock_exit(void)
{
	int i;

	rtos_thread_abort(&mclock_ctx.task);

	for (i = 0; i < MCLOCK_DOMAIN_PTP_RANGE; i++)
		mclock_gen_ptp_exit(&mclk_drv_h->gen_ptp[i]);

	mclk_drv_h = NULL;
}

static struct mclock_dev *__mclock_drv_find_device(mclock_t type, int domain)
{
	struct mclock_drv *drv = mclk_drv_h;
	struct mclock_dev *dev;
	struct slist_node *entry;

	for (entry = slist_first(&drv->mclock_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		dev = container_of(entry, struct mclock_dev, list_node);

		if (!(dev->flags & MCLOCK_FLAGS_FREE) && (dev->type == type) && (dev->domain == domain))
			goto found;
	}

	return NULL;

found:
	return dev;
}

static struct mclock_dev * __mclock_get_device(mclock_t type, int domain)
{
	struct mclock_drv *drv = mclk_drv_h;
	struct mclock_dev *dev;
	struct slist_node *entry;

	for (entry = slist_first(&drv->mclock_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		dev = container_of(entry, struct mclock_dev, list_node);

		if ((dev->flags & MCLOCK_FLAGS_FREE) && (dev->type == type) && (dev->domain == domain)) {
			dev->flags &= ~MCLOCK_FLAGS_FREE;
			return dev;
		}
	}

	return NULL;
}

static int __mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start, bool internal)
{
	struct mclock_dev *dev;
	int rc;
	bool is_media_clock_running;

	/* get reference to media clock */
	if (internal)
		dev = mtimer_dev->mclock_dev;
	else
		dev = __mclock_drv_find_device(mtimer_dev->type, mtimer_dev->domain);

	if (!dev) {
		rc = -1;
		goto err;
	}

	is_media_clock_running = dev->flags & MCLOCK_FLAGS_RUNNING;

	if (mtimer_start(mtimer_dev, start, dev->clk_timer, internal, is_media_clock_running)) {

		/* If the first timer in the list and media clock is running: add the wake up flag */
		if (slist_empty(&dev->mtimer_devices) & is_media_clock_running)
			dev->flags |= MCLOCK_FLAGS_WAKE_UP;

		slist_add_head(&dev->mtimer_devices, &mtimer_dev->list);

		mtimer_dev->mclock_dev = dev;
	} else if (internal) {
		/* Add the wake up flag.
		 * No need to check if list is not empty: if internal start it's definetly populated */
		dev->flags |= MCLOCK_FLAGS_WAKE_UP;
	}

	return 0;

err:
	return rc;
}

int mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start)
{
	int rc;

	if (!start->p || !start->q) {
		rc = -1;
		goto err;
	}

	rtos_mutex_lock(&mclk_drv_h->mutex, RTOS_WAIT_FOREVER);

	rc = __mclock_timer_start(mtimer_dev, start, false);

	rtos_mutex_unlock(&mclk_drv_h->mutex);

err:
	return rc;
}

static void __mclock_timer_stop(struct mtimer_dev *mtimer_dev, bool internal)
{
	struct mclock_dev *dev = mtimer_dev->mclock_dev;

	if (mtimer_stop(mtimer_dev, internal)) {

		slist_del(&dev->mtimer_devices, &mtimer_dev->list);

		if (slist_empty(&dev->mtimer_devices))
			dev->flags &= ~MCLOCK_FLAGS_WAKE_UP;

		mtimer_dev->mclock_dev = NULL;
	} else if (internal) {
		/* Clear the wake up flag.
		 * No need to check if list is not empty: if internal stop it's definetly populated */
		dev->flags &= ~MCLOCK_FLAGS_WAKE_UP;
	}
}

void mclock_timer_stop(struct mtimer_dev *mtimer_dev)
{
	rtos_mutex_lock(&mclk_drv_h->mutex, RTOS_WAIT_FOREVER);

	__mclock_timer_stop(mtimer_dev, false);

	rtos_mutex_unlock(&mclk_drv_h->mutex);
}

static void mclock_timer_start_all(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mtimer_dev *mtimer_dev;
	struct slist_node *entry;

	rtos_mutex_lock(&mclk_drv_h->mutex, RTOS_WAIT_FOREVER);

	for (entry = slist_first(&dev->mtimer_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		mtimer_dev = container_of(entry, struct mtimer_dev, list);
		__mclock_timer_start(mtimer_dev, NULL, true);
	}

	rtos_mutex_unlock(&mclk_drv_h->mutex);
}

static void mclock_timer_stop_all(struct mclock_dev *dev)
{
	struct mtimer_dev *mtimer_dev;
	struct slist_node *entry;

	rtos_mutex_lock(&mclk_drv_h->mutex, RTOS_WAIT_FOREVER);

	for (entry = slist_first(&dev->mtimer_devices); !slist_is_last(entry); entry = slist_next(entry)) {
		mtimer_dev = container_of(entry, struct mtimer_dev, list);
		__mclock_timer_stop(mtimer_dev, true);
	}

	rtos_mutex_unlock(&mclk_drv_h->mutex);
}

static int mclock_start(struct mclock_dev *dev, struct mclock_start *start)
{
	int rc;

	if (dev->start) {
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -1;
			goto err;
		}

		rc = dev->start(dev, start);
		if (!rc)
			dev->flags |= MCLOCK_FLAGS_RUNNING;

		mclock_timer_start_all(dev, start);

	} else {
		rc = -1;
	}

err:
	return rc;
}

static int mclock_stop(struct mclock_dev *dev)
{
	int rc;

	mclock_timer_stop_all(dev);

	if (dev->stop) {
		if (!(dev->flags & MCLOCK_FLAGS_RUNNING)) {
			rc = -1;
			goto err;
		}

		rc = dev->stop(dev);

		dev->flags &= ~MCLOCK_FLAGS_RUNNING;
	} else {
		rc = -1;
	}

err:
	return rc;
}

static int mclock_clean(struct mclock_dev *dev, struct mclock_clean *clean)
{
	int rc;

	if (dev->clean)
		rc = dev->clean(dev, clean);
	else
		rc = -1;

	return rc;
}

static int mclock_reset(struct mclock_dev *dev)
{
	int rc;

	if (dev->reset) {
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -1;
			goto err;
		}

		rc = dev->reset(dev);

	} else {
		rc = -1;
	}

err:
	return rc;
}

static void mclock_gconfig(struct mclock_dev *dev, struct mclock_gconfig *cfg)
{
	cfg->ts_src = dev->ts_src;
	cfg->ts_freq_p = dev->ts_freq_p;
	cfg->ts_freq_q = dev->ts_freq_q;
	cfg->array_addr = dev->sh_mem;
	cfg->array_size = dev->num_ts;
	cfg->timer_period = dev->timer_period;
}

static int mclock_sconfig(struct mclock_dev *dev, struct mclock_sconfig *cfg)
{
	int rc;

	if (dev->config)
		rc = dev->config(dev, cfg);
	else
		rc = -1;

	return rc;
}

static int mclock_open(struct mclock_dev *dev, int port)
{
	int rc;

	if (dev->open)
		rc = dev->open(dev, port);
	else
		rc = -1;

	return rc;
}

static void mclock_release(struct mclock_dev *dev)
{
	if (dev->stop && (dev->flags & MCLOCK_FLAGS_RUNNING)) {
		dev->stop(dev);
		dev->flags &= ~MCLOCK_FLAGS_RUNNING;
	}

	if (dev->release)
		dev->release(dev);
}

/**
 * Media Clock API functions
 */

__init int os_media_clock_rec_init(struct os_media_clock_rec *rec, int domain_id)
{
	struct mclock_dev *dev;
	struct mclock_gconfig hw_conf;
	mclock_t type;
	int port;
	int rc = 0;

	type = REC;
	port = 0;

	rtos_mutex_lock(&mclk_drv_h->mutex, RTOS_WAIT_FOREVER);

	dev = __mclock_get_device(type, domain_id);
	if (!dev) {
		rc = -1;
		goto err_unlock;
	}

	rtos_mutex_unlock(&mclk_drv_h->mutex);

	rec->mclock_dev = dev;
	mclock_open(rec->mclock_dev, port);

	mclock_gconfig(rec->mclock_dev, &hw_conf);

	rec->array_size = hw_conf.array_size;
	rec->array_addr = hw_conf.array_addr;

	os_log(LOG_INIT, "clock id %d, init done\n", domain_id);

	return 0;

err_unlock:
	rtos_mutex_unlock(&mclk_drv_h->mutex);

	return rc;
}

__exit void os_media_clock_rec_exit(struct os_media_clock_rec *rec)
{
	os_media_clock_rec_stop(rec);
	mclock_release(rec->mclock_dev);
}

int os_media_clock_rec_stop(struct os_media_clock_rec *rec)
{
	if (mclock_stop(rec->mclock_dev)) {
		os_log(LOG_ERR, "mclock_stop()\n");
		return -1;
	}

	return 0;
}

int os_media_clock_rec_reset(struct os_media_clock_rec *rec)
{
	if (mclock_reset(rec->mclock_dev)) {
		os_log(LOG_ERR, "mclock_reset()\n");
		return -1;
	}

	return 0;
}

int os_media_clock_rec_set_ts_freq(struct os_media_clock_rec *rec, unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	struct mclock_sconfig cfg;

	cfg.cmd = MCLOCK_CFG_FREQ;
	cfg.ts_freq.p = ts_freq_p;
	cfg.ts_freq.q = ts_freq_q;

	if (mclock_sconfig(rec->mclock_dev, &cfg)) {
		os_log(LOG_ERR, "mclock_sconfig()\n");
		return -1;
	}

	return 0;
}

static int media_clock_rec_set_ts_src(struct os_media_clock_rec *os, mclock_ts_src_t ts_src)
{
	struct mclock_sconfig cfg;

	cfg.cmd = MCLOCK_CFG_TS_SRC;
	cfg.ts_src = ts_src;

	if (mclock_sconfig(os->mclock_dev, &cfg)) {
		os_log(LOG_ERR, "mclock_sconfig()\n");
		return -1;
	}

	return 0;
}

int os_media_clock_rec_set_ext_ts(struct os_media_clock_rec *rec)
{
	return media_clock_rec_set_ts_src(rec, TS_EXTERNAL);
}

int os_media_clock_rec_set_ptp_sync(struct os_media_clock_rec *os)
{
	return media_clock_rec_set_ts_src(os, TS_INTERNAL);
}

int os_media_clock_rec_start(struct os_media_clock_rec *rec, u32 ts_0, u32 ts_1)
{
	struct mclock_start start;

	start.ts_0 = ts_0;
	start.ts_1 = ts_1;

	if (mclock_start(rec->mclock_dev, &start) < 0) {
		os_log(LOG_ERR, "mclock_start()\n");
		return -1;
	}

	return 0;
}

os_media_clock_rec_state_t os_media_clock_rec_clean(struct os_media_clock_rec *rec, unsigned int *nb_clean)
{
	struct mclock_clean clean;
	os_media_clock_rec_state_t rc = OS_MCR_RUNNING;

	if (mclock_clean(rec->mclock_dev, &clean) < 0) {
		os_log(LOG_ERR, "mclock_clean()\n");
		return OS_MCR_ERROR;
	}

	*nb_clean = clean.nb_clean;

	if (clean.status == MCLOCK_RUNNING_LOCKED)
		rc = OS_MCR_RUNNING_LOCKED;
	else if (clean.status == MCLOCK_STOPPED)
		rc = OS_MCR_ERROR;

	return rc;
}

void os_media_clock_gen_exit(struct os_media_clock_gen *gen)
{
	mclock_release(gen->mclock_dev);
}

int os_media_clock_gen_init(struct os_media_clock_gen *gen, int id, unsigned int is_hw)
{
	struct mclock_dev *dev;
	struct mclock_gconfig hw_conf;
	mclock_t type;
	int port;
	int rc = 0;

	if (is_hw)
		type = GEN;
	else
		type = PTP;

	port = 0;

	dev = __mclock_get_device(type, id);
	if (!dev) {
		rc = -1;
		goto err;
	}

	gen->mclock_dev = dev;
	mclock_open(gen->mclock_dev, port);

	mclock_gconfig(gen->mclock_dev, &hw_conf);

	gen->array_size = hw_conf.array_size;

	gen->ts_freq_p = hw_conf.ts_freq_p;
	gen->ts_freq_q = hw_conf.ts_freq_q;
	gen->timer_period = hw_conf.timer_period;

	if (os_media_clock_gen_reset(gen) < 0) {
		os_log(LOG_ERR,"os_media_clock_gen_reset failed\n");
		goto err_reset;
	}

	gen->array_addr = hw_conf.array_addr;
	if (!gen->array_addr) {
		os_log(LOG_ERR,"mmap failed\n");
		goto err_mmap;
	}

	gen->w_idx = (unsigned int *)((char *)gen->array_addr + gen->array_size * sizeof(unsigned int));
	gen->ptp = gen->w_idx + 1;
	gen->count = gen->w_idx + 2;

	os_log(LOG_INFO, "hw_source(%p) init done\n", gen);


err_mmap:
err_reset:
err:
	return rc;
}

void os_media_clock_gen_ts_update(struct os_media_clock_gen *gen, unsigned int *w_idx, unsigned int *count)
{
	*count = *gen->count;
	*w_idx = *gen->w_idx;
}

int os_media_clock_gen_stop(struct os_media_clock_gen *gen)
{
	if (mclock_stop(gen->mclock_dev)) {
		os_log(LOG_ERR, "mclock_stop()\n");
		return -1;
	}

	return 0;
}

int os_media_clock_gen_start(struct os_media_clock_gen *gen, u32 *write_index)
{
	struct mclock_start dummy;

	if (mclock_start(gen->mclock_dev, &dummy) < 0) {
		os_log(LOG_ERR, "mclock_start()\n");
		return -1;
	}

	*write_index = *(gen->w_idx);

	return 0;
}

int os_media_clock_gen_reset(struct os_media_clock_gen *gen)
{
	if (mclock_reset(gen->mclock_dev)) {
		os_log(LOG_ERR, "mclock_reset()\n");
		return -1;
	}

	return 0;
}
