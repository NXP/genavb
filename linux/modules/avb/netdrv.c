/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/poll.h>

#include "netdrv.h"
#include "queue.h"
#include "avbdrv.h"

#include "net_socket.h"

#define NET_READ_MAX_BUFFERS	32
#define NET_WRITE_BATCH		16

static ssize_t netdrv_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
	struct net_socket *sock = file->private_data;
	int rc = 0;
	unsigned long addr[NET_WRITE_BATCH];
	unsigned int write = 0, n_now, num;

	if (*off)
		return -EPIPE;

	num = len / sizeof(unsigned long);

	while (write < num) {
		n_now = num - write;
		if (n_now > NET_WRITE_BATCH)
			n_now = NET_WRITE_BATCH;

		if (copy_from_user(addr, &((unsigned long *)buf)[write], n_now * sizeof(unsigned long)))
			return -EFAULT;

		rc = socket_write(sock, addr, n_now);
		if (rc <= 0)
			break;

		write += rc;
	}

	/* Write failure */
	if (!write)
		return rc;

	return write * sizeof(unsigned long);
}

static ssize_t netdrv_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
	struct net_socket *sock = file->private_data;
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	int rc = 0;
	int n;
	unsigned long addr[NET_READ_MAX_BUFFERS];

	if (*off)
		return -EPIPE;

	n = len / sizeof(unsigned long);

	/* The maximum number of buffers per read is capped to NET_READ_MAX_BUFFERS*/
	if (n > NET_READ_MAX_BUFFERS)
		n = NET_READ_MAX_BUFFERS;

	rc = socket_read(sock, addr, n);
	if (rc <= 0)
		return rc;

	if (copy_to_user(buf, addr, rc * sizeof(unsigned long))) {
		/* Failed to copy to userspace, free dequeued buffers*/
		pool_dma_free_array_shmem(&avb->buf_pool, addr, rc);
		return -EFAULT;
	}

	return rc * sizeof(unsigned long);
}


/* Queue character device callbacks */
static long netdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct net_socket *sock = file->private_data;
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct net_address addr;
	struct net_sr_config sr_config;
	struct net_ts_desc ts_desc;
	struct net_mc_address mc_addr;
	unsigned int tx_available;
	struct net_sr_class_cfg net_sr_class_cfg;
	struct net_port_status port_status;
	struct net_set_option option;
	int rc = 0;

	//	pr_info("%s: file: %p cmd: %u, arg: %lu\n", __func__, file, cmd, arg);

	switch (cmd) {
	case NET_IOC_BIND:
		if (copy_from_user(&addr, (void *)arg, sizeof(struct net_address))) {
			rc = -EFAULT;
			break;
		}

		rc = socket_bind(sock, &addr);

		break;

	case NET_IOC_CONNECT:
		if (copy_from_user(&addr, (void *)arg, sizeof(struct net_address))) {
			rc = -EFAULT;
			break;
		}

		rc = socket_connect(sock, &addr);

		break;

	case NET_IOC_SR_CONFIG:
		if (copy_from_user(&sr_config, (void *)arg, sizeof(struct net_sr_config))) {
			rc = -EFAULT;
			break;
		}

		rc = net_qos_sr_config(&avb->qos, &sr_config);

		break;

	case NET_IOC_TS_GET:
		rc = socket_read_ts(sock, &ts_desc);
		if (rc < 0)
			break;

		rc = copy_to_user((void *)arg, &ts_desc, sizeof(struct net_ts_desc));

		break;

	case NET_IOC_ADDMULTI:
		if (copy_from_user(&mc_addr, (void *)arg, sizeof(struct net_mc_address))) {
			rc = -EFAULT;
			break;
		}

		rc = socket_add_multi(sock, &mc_addr);

		break;

	case NET_IOC_DELMULTI:
		if (copy_from_user(&mc_addr, (void *)arg, sizeof(struct net_mc_address))) {
			rc = -EFAULT;
			break;
		}

		rc = socket_del_multi(sock, &mc_addr);

		break;

	case NET_IOC_SET_OPTION:
		if (copy_from_user(&option, (void *)arg, sizeof(struct net_set_option))) {
			rc = -EFAULT;
			break;
		}

		rc = socket_set_option(sock, &option);

		break;

	case NET_IOC_GET_TX_AVAILABLE:
		tx_available = socket_tx_available(sock);

		rc = put_user(tx_available, (unsigned int  *)arg);

		break;

	case NET_IOC_SR_CLASS_CONFIG:

		if (copy_from_user(&net_sr_class_cfg, (void *)arg, sizeof(struct net_sr_class_cfg))) {
			rc = -EFAULT;
			break;
		}

		rc = net_qos_sr_class_configure(avb, &net_sr_class_cfg);

		break;

	case NET_IOC_PORT_STATUS:
		if (copy_from_user(&port_status, (void *)arg, sizeof(struct net_port_status))) {
			rc = -EFAULT;
			break;
		}

		if (socket_port_status(sock, &port_status) < 0) {
			rc = -EINVAL;
			break;
		}

		rc = copy_to_user((void *)arg, &port_status, sizeof(struct net_port_status));

		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static unsigned int netdrv_poll(struct file *file, poll_table *poll)
{
	struct net_socket *sock = file->private_data;
	unsigned int mask = 0;

//	pr_info("%s: file: %p\n", __func__, file);

	poll_wait(file, &sock->wait, poll);

	if (sock->flags & SOCKET_FLAGS_RX) {
		if (sock->flags & SOCKET_FLAGS_RX_BATCHING) {
			if (test_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags))
				mask |= POLLIN | POLLRDNORM;
		} else if (queue_pending(&sock->queue))
			mask |= POLLIN | POLLRDNORM;
	} else {
		if (sock->flags & SOCKET_FLAGS_FLOW_CONTROL) {
			/* at least 25% of the queue is free, now we can wake-up the thread and disable the flow control */
			if (queue_available(&sock->queue) >= (sock->queue.size >> 2)) {
				mask |= POLLOUT | POLLWRNORM;
				clear_bit(SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT, &sock->qos_queue->atomic_flags);
			} else {
				set_bit(SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT, &sock->qos_queue->atomic_flags);
			}
		} else if (sock->flags & SOCKET_FLAGS_TX_TS_ENABLED) {
			if (queue_pending(&sock->tx_ts_queue)) {
				mask |= POLLIN | POLLRDNORM;
			}
		}
	}

//	pr_info("%s: %x\n", __func__, mask);

	return mask;
}


static int netdrv_release(struct inode *in, struct file *file)
{
	struct net_socket *sock = file->private_data;

	socket_close(sock);

	file->private_data = NULL;

	return 0;
}

static int netdrv_open(struct inode *in, struct file *file)
{
	struct net_drv *drv = container_of(in->i_cdev, struct net_drv, cdev);
	struct net_socket *sock;
	int rc = -ENOMEM;
	int minor = MINOR(in->i_rdev);

	sock = socket_open(minor == NETDRV_NET_RX_MINOR);
	if (!sock)
		goto err_open;

	sock->drv = drv;
	file->private_data = sock;

	return 0;

err_open:
	return rc;
}

static const struct file_operations netdrv_fops = {
	.owner = THIS_MODULE,
	.open = netdrv_open,
	.release = netdrv_release,
	.write = netdrv_write,
	.read = netdrv_read,
	.unlocked_ioctl = netdrv_ioctl,
	.poll = netdrv_poll,
};

int netdrv_init(struct net_drv *drv)
{
	int rc;

	pr_info("%s: %p\n", __func__, drv);

	INIT_LIST_HEAD(&drv->batching_sync_list);
	INIT_LIST_HEAD(&drv->batching_async_list);
	INIT_LIST_HEAD(&drv->no_batching_list);

	rc = alloc_chrdev_region(&drv->devno, NETDRV_MINOR, NETDRV_MINOR_COUNT, NETDRV_NAME);
	if (rc < 0) {
		pr_err("%s: alloc_chrdev_region() failed\n", __func__);
		goto err_alloc_chrdev;
	}

	cdev_init(&drv->cdev, &netdrv_fops);
	drv->cdev.owner = THIS_MODULE;

	rc = cdev_add(&drv->cdev, drv->devno, NETDRV_MINOR_COUNT);
	if (rc < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_add;
	}

	return 0;

err_cdev_add:
	unregister_chrdev_region(drv->devno, NETDRV_MINOR_COUNT);

err_alloc_chrdev:
	return rc;
}

void netdrv_exit(struct net_drv *drv)
{
	pr_info("%s: %p\n", __func__, drv);

	cdev_del(&drv->cdev);
	unregister_chrdev_region(drv->devno, NETDRV_MINOR_COUNT);
}
