/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _OPCUA_SERVER_H_
#define _OPCUA_SERVER_H_

#include "../cyclic_task.h"

struct opcua_server;

#ifdef OPCUA_SUPPORT
void opcua_init_params(struct cyclic_task *task);
void opcua_update_cyclic_socket(struct socket *sock);
void opcua_update_net_socket_stats(struct net_socket *sock);
void opcua_update_task_stats(struct tsn_task_stats *task);
int opcua_server_init(void);
void opcua_server_exit(void);
#else
static inline int opcua_server_init(void)
{
	return 0;
};
static inline void opcua_init_params(struct cyclic_task *task) { return; };
static inline void opcua_update_cyclic_socket(struct socket *sock) { return; };
static inline void opcua_update_net_socket_stats(struct net_socket *sock) { return; };
static inline void opcua_update_task_stats(struct tsn_task_stats *task) { return; };
static inline void opcua_server_exit(void) { return; };
#endif

#endif /* _OPCUA_SERVER_H_ */
