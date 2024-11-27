/* Copyright 2019-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EBPF_H
#define _EBPF_H

/* This file contains helper definitions to:
 *   * write XDP programs
 *   * load XDP programs through iproute2
 */

#define BPF_MAP_TYPE_HASH		1
#define BPF_MAP_TYPE_ARRAY		2
#define BPF_MAP_TYPE_PROG_ARRAY		3
#define BPF_MAP_TYPE_PERCPU_HASH	5
#define BPF_MAP_TYPE_PERCPU_ARRAY	6
#define BPF_MAP_TYPE_XSKMAP		17

#define PIN_NONE	0
#define PIN_GLOBAL_NS	2

#ifndef __section
# define __section(NAME)						\
	__attribute__((section(NAME), used))
#endif

#ifndef __inline
# define __inline							\
	inline __attribute__((always_inline))
#endif


/* ELF map definition */
struct bpf_elf_map {
	uint32_t type;
	uint32_t size_key;
	uint32_t size_value;
	uint32_t max_elem;
	uint32_t flags;
	uint32_t id;
	uint32_t pinning;
	uint32_t inner_id;
	uint32_t inner_idx;
};

enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};

struct xdp_ctx {
	uint32_t data;
	uint32_t data_end;
};

#define BPF_FUNC_map_lookup_elem	1
#define BPF_FUNC_map_update_elem	2
#define BPF_FUNC_trace_printk		6
#define BPF_FUNC_tail_call		12
#define BPF_FUNC_redirect		23
#define BPF_FUNC_xdp_adjust_head	44
#define BPF_FUNC_redirect_map		51

static void *(*bpf_map_lookup_elem)(void *map, void *key) =
	(void *) BPF_FUNC_map_lookup_elem;
static int (*bpf_map_update_elem)(void *map, void *key, void *value,
				  unsigned long long flags) =
	(void *) BPF_FUNC_map_update_elem;
static void (*bpf_tail_call)(void *ctx, void *map, int index) =
	(void *) BPF_FUNC_tail_call;
static int (*bpf_redirect)(int ifindex, int flags) =
	(void *) BPF_FUNC_redirect;
static int (*bpf_xdp_adjust_head)(void *ctx, int offset) =
	(void *) BPF_FUNC_xdp_adjust_head;
static int (*bpf_trace_printk)(const char *fmt, int fmt_size, ...) =
	(void *) BPF_FUNC_trace_printk;
static int (*bpf_redirect_map)(void *map, void *key, int flags) =
	(void *) BPF_FUNC_redirect_map;

#endif /* _EBPF_H */
