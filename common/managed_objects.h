/*
 * Copyright 2018, 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Managed objects handling functions
 @details
*/

#ifndef _MANAGED_OBJECTS_H_
#define _MANAGED_OBJECTS_H_

#include "os/sys_types.h"

enum node_type {
	NODE_MODULE,
	NODE_CONTAINER,
	NODE_LIST,
	NODE_LIST_ENTRY,
	NODE_LEAF,
	NODE_LEAF_LIST,
};

enum leaf_type {
	LEAF_TYPE_MIN,
	LEAF_BOOL = LEAF_TYPE_MIN,
	LEAF_UINT8,
	LEAF_UINT16,
	LEAF_UINT32,
	LEAF_UINT64,
	LEAF_INT8,
	LEAF_INT16,
	LEAF_INT32,
	LEAF_INT64,
	LEAF_DOUBLE,
	LEAF_SCALED_NS,
	LEAF_USCALED_NS,
	LEAF_CLOCK_IDENTITY,
	LEAF_PORT_IDENTITY,
	LEAF_MAC_ADDRESS,
	LEAF_TYPE_MAX = LEAF_MAC_ADDRESS,
};

enum leaf_flags {
	LEAF_R = 0x1,
	LEAF_W = 0x2,
	LEAF_RW = 0x3
};

enum node_operation {
	NODE_GET = 0,
	NODE_SET
};

enum node_status {
	NODE_STATUS_OK = 0,
	NODE_STATUS_ERR_LENGTH = 1,
	NODE_STATUS_ERR_ID = 2,
	NODE_STATUS_ERR_TYPE = 3,
	NODE_STATUS_ERR_READ = 4,
	NODE_STATUS_ERR_WRITE = 5,
	NODE_STATUS_ERR_MAX_CHILD = 6,
	NODE_STATUS_ERR_KEY_ID = 7,
	NODE_STATUS_ERR_KEY_LENGTH = 8,
};

#define LIST_KEY_MAX	4

struct entry_key {
	uint16_t id;
	uint16_t length;
	void *val;
};

struct node {
	enum node_type type;
	const char *name;
	unsigned int max_child;
	struct node *child[]; /* Storage must be provided just below when declaring the actual node instances */
};

struct module {
	struct node n;
};

struct container {
	struct node n;
};

struct list {
	uint16_t key_id[LIST_KEY_MAX];
	unsigned int max_key;
	uintptr_t (*dynamic_handler)(void *, uintptr_t);
	void *dynamic_handler_data;
	struct node n;
};

struct list_entry {
	struct node n;
};

struct leaf {
	struct node n;
	enum leaf_flags flags; /* R, W, RW */
	enum leaf_type type;
	uint8_t *val;
	void (*handler)(void *, struct leaf *, enum node_operation, uint8_t *, uintptr_t);
	void *handler_data;
};

struct leaf_list {
	struct node n;
	enum leaf_type type;
	void *val;
};

struct  __attribute__((packed)) node_header {
	uint16_t id;
	uint16_t length;
};

struct  __attribute__((packed)) node_header_status {
	uint16_t id;
	uint16_t length;
	uint16_t status;
};

void node_array_link(struct node *parent, struct node *child);
void module_init(struct module *module, const char *name);
struct node *container_init(struct container *container, const char *name);
struct node *list_init(struct list *list, const char *name, uintptr_t (*handler)(void *, uintptr_t), void *handler_data, uint16_t num_keys, uint16_t *keys_id);
struct node *list_entry_init(struct list_entry *entry, const char *name);
struct node *leaf_init(struct leaf *leaf, const char *name, enum leaf_type type, enum leaf_flags flags, void *storage,
			void (*handler)(void *, struct leaf *, enum node_operation, uint8_t *, uintptr_t), void *handler_data);


/* Managed object data tree definition helper macros */
#define MODULE(_name, _child_n, _childs)	\
struct _name {\
	struct module node;\
	struct node *child[_child_n];	/* child node storage array, must be below the node declaration */\
	_childs\
};

#define CONTAINER(_name, _child_n, _childs)	\
struct {\
	struct container node;\
	struct node *child[_child_n];	/* child node storage array, must be below the node declaration */\
	_childs\
} _name

#define LIST(_name, _child_n, _childs)	\
struct {\
	struct list node;\
	struct node *child[_child_n];	/* child node storage array, must be below the node declaration */\
	_childs\
} _name

#define LIST_ENTRY(_name, _n, _child_n, _childs)	\
struct {\
	struct list_entry node;\
	struct node *child[_child_n];	/* child node storage array, must be below the node declaration */\
	_childs\
} _name[_n]

#define LEAF(_name)	\
struct {\
	struct leaf node;\
} _name

/* Managed object data tree initialization helper macros */
#define MODULE_INIT(_module, _name)	module_init(&(_module)->node, _name);

#define CONTAINER_INIT(_parent, _container)	{ \
	struct node *n = container_init(&((_parent)->_container.node), #_container);\
	node_array_link(&(_parent)->node.n, n);\
}

#define LIST_INIT(_parent, _list, num_keys, keys_id)	{ \
	struct node *n = list_init(&((_parent)->_list.node), #_list, NULL, NULL, num_keys, keys_id);\
	node_array_link(&(_parent)->node.n, n);\
}

#define LIST_DYNAMIC_INIT(_parent, _list, _handler, _handler_data, num_keys, keys_id)	{ \
	struct node *n = list_init(&((_parent)->_list.node), #_list, _handler, _handler_data, num_keys, keys_id);\
	node_array_link(&(_parent)->node.n, n);\
}

#define LIST_ENTRY_INIT(_parent, _entry)	{\
	struct node *n = list_entry_init(&((_parent)->_entry.node), #_entry);\
	node_array_link(&(_parent)->node.n, n);\
}

#define LEAF_INIT(_parent, _leaf, _type, _flags, _storage, _set_handler, _set_data)	{\
	struct node *n = leaf_init(&((_parent)->_leaf.node), #_leaf, _type, _flags, _storage, _set_handler, _set_data);\
	node_array_link(&(_parent)->node.n, n);\
}

uint8_t *module_iterate(struct module *m, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end);

#endif /* _MANAGED_OBJECTS_H_ */
