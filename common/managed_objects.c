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

#include "managed_objects.h"

#include "os/string.h"

#include "common/types.h"
#include "common/log.h"
#include "common/ptp.h"


static uint8_t *child_iterate(struct node *n, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end, uintptr_t base);

const unsigned int leaf_object_size[] = {
	[LEAF_SCALED_NS] = sizeof(struct ptp_scaled_ns),
	[LEAF_USCALED_NS] = sizeof(struct ptp_u_scaled_ns),
	[LEAF_PORT_IDENTITY] = sizeof(struct ptp_port_identity),
	[LEAF_UINT64] = sizeof(uint64_t),
	[LEAF_INT64] = sizeof(int64_t),
	[LEAF_CLOCK_IDENTITY] = sizeof(struct ptp_clock_identity),
	[LEAF_MAC_ADDRESS] = sizeof(uint8_t)*6,
	[LEAF_DOUBLE] = sizeof(double),
	[LEAF_UINT32] = sizeof(uint32_t),
	[LEAF_INT32]= sizeof(int32_t),
	[LEAF_UINT16] = sizeof(uint16_t),
	[LEAF_INT16] = sizeof(int16_t),
	[LEAF_UINT8] = sizeof(uint8_t),
	[LEAF_INT8] = sizeof(int8_t),
	[LEAF_BOOL] = sizeof(uint8_t),
};

void node_array_link(struct node *parent, struct node *child)
{
	parent->child[parent->max_child] = child;
	parent->max_child++;
}

static void node_init(struct node *node, const char *name, enum node_type type)
{
	node->type = type;
	node->max_child = 0;
	node->name = name;
}

void module_init(struct module *module, const char *name)
{
	node_init(&module->n, name, NODE_MODULE);
}

struct node *container_init(struct container *container, const char *name)
{
	node_init(&container->n, name, NODE_CONTAINER);

	return &container->n;
}

struct node *list_init(struct list *list, const char *name, uintptr_t (*handler)(void *, uintptr_t), void *handler_data, uint16_t num_keys, uint16_t *keys_id)
{
	int i;

	node_init(&list->n, name, NODE_LIST);

	list->dynamic_handler = handler;
	list->dynamic_handler_data = handler_data;

	list->max_key = min(num_keys, LIST_KEY_MAX);
	for (i = 0; i < list->max_key; i++)
		list->key_id[i] = keys_id[i];

	return &list->n;
}

struct node *list_entry_init(struct list_entry *entry, const char *name)
{
	node_init(&entry->n, name, NODE_LIST_ENTRY);

	return &entry->n;
}

struct node *leaf_init(struct leaf *leaf, const char *name, enum leaf_type type, enum leaf_flags flags, void *storage,
			void (*handler)(void *, struct leaf *, enum node_operation, uint8_t *, uintptr_t), void *handler_data)
{
	node_init(&leaf->n, name, NODE_LEAF);

	leaf->type = type;
	leaf->flags = flags;
	leaf->val = storage;
	leaf->handler = handler;
	leaf->handler_data = handler_data;

	return &leaf->n;
}

static struct node_header *get_header(uint8_t *buf, uint8_t *end)
{
	struct node_header *hdr;

	if ((end - buf) < sizeof(struct node_header))
		return NULL;

	hdr = (struct node_header *)buf;

	os_log(LOG_DEBUG, "id: %u, length: %u\n", hdr->id, hdr->length);

	return hdr;
}

static struct node_header_status *put_header_status_start(uint8_t *buf, uint8_t *end, uint16_t id)
{
	struct node_header_status *hdr;

	if ((end - buf) < sizeof(struct node_header_status))
		return NULL;

	hdr = (struct node_header_status *)buf;
	hdr->id = id;
	hdr->length = sizeof(uint16_t);
	hdr->status = 0;

	return hdr;
}

static uint8_t *put_header_status_end(struct node_header_status *hdr, uint16_t status, uint16_t length)
{
	hdr->length += length;
	hdr->status = status;

	return (uint8_t *)(hdr + 1);
}

static uint8_t *put_header_status(uint8_t *buf, uint8_t *end, uint16_t id, uint16_t status)
{
	struct node_header_status *hdr;

	hdr = put_header_status_start(buf, end, id);
	if (!hdr)
		return NULL;

	return put_header_status_end(hdr, status, 0);
}

static void leaf_bool_get(struct leaf *l, uint8_t *out, uintptr_t base)
{
	out[0] = *((bool *)(base + l->val));
}

static void _leaf_get(struct leaf *l, uint8_t *out, unsigned int size, uintptr_t base)
{
	os_memcpy(out, (base + l->val), size);
}

static uint8_t *leaf_get(struct node *n, unsigned int id, uint8_t *out, uint8_t *out_end, uintptr_t base, uint16_t *status)
{
	struct leaf *l = container_of(n, struct leaf, n);
	struct node_header_status *hdr;
	unsigned int object_size;
	unsigned int leaf_data_length = 0;

	*status = NODE_STATUS_OK;

	if ((!l->val && !base) || !(l->flags & LEAF_R)) {
		/* Leaf can not be read */
		*status = NODE_STATUS_ERR_LENGTH;
		out = put_header_status(out, out_end, id, *status);
		goto out;
	}

	if ((l->type < LEAF_TYPE_MIN) || (l->type > LEAF_TYPE_MAX)) {
		/* unknown leaf type, stop parsing */
		*status = NODE_STATUS_ERR_TYPE;
		out = put_header_status(out, out_end, id, *status);
		goto out;
	}

	hdr = put_header_status_start(out, out_end, id);
	if (!hdr) {
		/* no room for node response, stop parsing */
		*status = NODE_STATUS_ERR_LENGTH;
		goto out;
	}

	object_size = leaf_object_size[l->type];

	out = (uint8_t *)(hdr + 1);

	/* FIXME should be refactored below with code from leaf_compare */
	if ((out_end - out) < object_size) {
		*status = NODE_STATUS_ERR_LENGTH;
		goto end;
	}

	if (l->handler) {
		l->handler(l->handler_data, l, NODE_GET, out, base);
	} else {
		if (l->type == LEAF_BOOL)
			leaf_bool_get(l, out, base);
		else
			_leaf_get(l, out, object_size, base);
	}

	out += object_size;
	leaf_data_length = object_size;

end:
	put_header_status_end(hdr, *status, leaf_data_length);

out:
	return out;
}

static void leaf_bool_set(struct leaf *l, uint8_t *in, uintptr_t base)
{
	if (in[0])
		((bool *)(base + l->val))[0] = 1;
	else
		((bool *)(base + l->val))[0] = 0;
}

static void _leaf_set(struct leaf *l, uint8_t *in, unsigned int size, uintptr_t base)
{
	os_memcpy((base + l->val), in, size);
}

static void *leaf_set(struct node *n, unsigned int id, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end, uintptr_t base)
{
	struct leaf *l = container_of(n, struct leaf, n);
	struct node_header_status *hdr;
	unsigned int object_size;
	uint16_t status;

	if ((!l->val && !base) || !(l->flags & LEAF_W)) {
		out = put_header_status(out, out_end, id, NODE_STATUS_ERR_WRITE);
		goto out;
	}

	if ((l->type < LEAF_TYPE_MIN) || (l->type > LEAF_TYPE_MAX)) {
		/* unknown leaf type, stop parsing */
		out = put_header_status(out, out_end, id, NODE_STATUS_ERR_TYPE);
		goto out;
	}

	hdr = put_header_status_start(out, out_end, id);
	if (!hdr) {
		/* no room for node response, stop parsing */
		goto out;
	}

	object_size = leaf_object_size[l->type];

	if ((in_end - in) < object_size) {
		status = NODE_STATUS_ERR_LENGTH;
		goto end;
	}

	status = NODE_STATUS_OK;

	if (l->handler) {
		l->handler(l->handler_data, l, NODE_SET, in, base);
	} else {
		if (l->type == LEAF_BOOL)
			leaf_bool_set(l, in, base);
		else
			_leaf_set(l, in, object_size, base);
	}

end:
	out = put_header_status_end(hdr, status, 0);

out:
	return out;
}

static void *leaf_list_get(struct node *n, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end)
{
	return out;
}

static void *leaf_list_set(struct node *n, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end)
{
	return out;
}

static int leaf_compare(struct leaf *leaf, uintptr_t base, void *key)
{
	uint8_t leaf_value[12];

	if (leaf->handler)
		leaf->handler(NULL, leaf, NODE_GET, leaf_value, base);
	else {
		if (leaf->type == LEAF_BOOL)
			leaf_bool_get(leaf, leaf_value, base);
		else
			_leaf_get(leaf, leaf_value, leaf_object_size[leaf->type], base);
	}

	return os_memcmp(key, leaf_value, leaf_object_size[leaf->type]);
}

static bool list_entry_match(struct list *l, struct node *child, uintptr_t base, struct entry_key *keys)
{
	struct leaf *leaf;
	int i;

	for (i = 0; i < l->max_key; i++) {
		/* key always matches if wildcard */
		if (!keys[i].length)
			continue;

		/* else retrieve entry's leaf and check if key matches */
		leaf = container_of(child->child[keys[i].id], struct leaf, n);

		if (leaf_compare(leaf, base, keys[i].val) != 0)
			return false;
	}

	return true;
}

static int list_entry_extract_keys(struct node *n, struct list *l, struct entry_key *keys, uint8_t **in, uint8_t *in_end, uint8_t **out, uint8_t *out_end)
{
	struct node_header *key_child_hdr;
	struct node *child, *_child;
	struct leaf *_leaf;
	int i;

	for (i = 0; i < l->max_key; i++) {
		key_child_hdr = get_header(*in, in_end);
		if (!key_child_hdr) {
			/* no room for child header, stop parsing */
			goto err;
		}

		*in = (uint8_t *)(key_child_hdr + 1);

		if (key_child_hdr->id != l->key_id[i]) {
			/* Invalid key id */
			*out = put_header_status(*out, out_end, key_child_hdr->id, NODE_STATUS_ERR_KEY_ID);
			goto err;
		}

		if ((in_end - *in) < key_child_hdr->length) {
			/* Invalid key length */
			*out = put_header_status(*out, out_end, key_child_hdr->id, NODE_STATUS_ERR_LENGTH);
			goto err;
		}

		/* sanity checks on key */
		if (l->dynamic_handler) {
			child = n->child[0];
			_child = child->child[key_child_hdr->id];

			if (_child->type != NODE_LEAF) {
				*out = put_header_status(*out, out_end, key_child_hdr->id, NODE_STATUS_ERR_TYPE);
				goto err;
			}

			_leaf = container_of(_child, struct leaf, n);

			/* key length can be 0 for wildcard */
			if (key_child_hdr->length != 0 && key_child_hdr->length != leaf_object_size[_leaf->type]) {
				*out = put_header_status(*out, out_end, key_child_hdr->id, NODE_STATUS_ERR_KEY_LENGTH);
				goto err;
			}
		} else {
			if (key_child_hdr->length != sizeof(uint16_t)) {
				*out = put_header_status(*out, out_end, key_child_hdr->id, NODE_STATUS_ERR_KEY_LENGTH);
				goto err;
			}

			/* for static array, the key value is the index in the array */
			if (*(uint16_t *)*in >= n->max_child) {
				*out = put_header_status(*out, out_end, key_child_hdr->id, NODE_STATUS_ERR_MAX_CHILD);
				goto err;
			}
		}

		/* save key informations */
		keys[i].id = key_child_hdr->id;
		keys[i].length = key_child_hdr->length;
		keys[i].val = (void *)*in;

		*in += key_child_hdr->length;
	}

	return 0;

err:
	return -1;
}

static uint8_t *list_entry_iterate(struct node *n, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end)
{
	struct node *child;
	uintptr_t dynamic_entry_base = 0;
	struct entry_key keys[LIST_KEY_MAX] = {0};
	struct list *l = container_of(n, struct list, n);
	uint16_t status;
	int i;

	/*
	* extract list's keys
	*/
	if (list_entry_extract_keys(n, l, keys, &in, in_end, &out, out_end) < 0)
		goto end;

	if (l->dynamic_handler) {
		/*
		* dynamic list, always at index 0 in the child array
		*/
		child = n->child[0];

		/* start from head of the list */
		dynamic_entry_base = 0;

		/* iterate through the dynamic list to look for entries matching the look-up keys */
		while ((dynamic_entry_base = l->dynamic_handler(l->dynamic_handler_data, dynamic_entry_base))) {
			if (!list_entry_match(l, child, dynamic_entry_base, keys))
				continue;

			/* entry matched, output keys */
			for (i = 0; i < l->max_key; i++) {
				out = leaf_get(child->child[keys[i].id], keys[i].id, out, out_end, dynamic_entry_base, &status);
				if (status != NODE_STATUS_OK)
					goto end;
			}

			out = child_iterate(child, operation, in, in_end, out, out_end, dynamic_entry_base);
		}
	} else {
		/*
		* static list, with key value as index in the child array
		*/
		child = n->child[*(uint16_t *)keys[0].val];

		out = leaf_get(child->child[keys[0].id], keys[0].id, out, out_end, 0, &status);
		if (status != NODE_STATUS_OK)
			goto end;

		out = child_iterate(child, operation, in, in_end, out, out_end, 0);
	}

end:
	return out;
}

static uint8_t *list_iterate(struct node *n, unsigned int id, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end)
{
	struct node_header_status *hdr;
	uint8_t *out_base;
	unsigned int child_total_length;

	hdr = put_header_status_start(out, out_end, id);
	if (!hdr) {
		/* no room for node response, stop parsing */
		goto end;
	}

	out_base = (uint8_t *)(hdr + 1);

	out = list_entry_iterate(n, operation, in, in_end, out_base, out_end);

	child_total_length = out - out_base;

	put_header_status_end(hdr, NODE_STATUS_OK, child_total_length);

end:
	return out;
}

static uint8_t *node_iterate(struct node *n, unsigned int id, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end)
{
	struct node_header_status *hdr;
	uint8_t *out_base;
	unsigned int child_total_length;

	hdr = put_header_status_start(out, out_end, id);
	if (!hdr) {
		/* no room for node response, stop parsing */
		goto end;
	}

	out_base = (uint8_t *)(hdr + 1);

	out = child_iterate(n, operation, in, in_end, out_base, out_end, 0);

	child_total_length = out - out_base;

	put_header_status_end(hdr, NODE_STATUS_OK, child_total_length);

end:
	return out;
}

/* The parent validates the child header
 * - enough room for the header, if not stop parsing
 * - enough room for the length specified in the header, if not add child error to the response, stop parsing
 * - known id, if not add child error to the response, skip child
 */
static uint8_t *child_iterate(struct node *n, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end, uintptr_t base)
{
	struct node_header *child_hdr;
	struct node *child;
	uint16_t status;

	/* Process childs in a loop */
	while (in < in_end) {
		child_hdr = get_header(in, in_end);
		if (!child_hdr) {
			/* no room for child header, stop parsing */
			break;
		}

		in = (uint8_t *)(child_hdr + 1);

		if ((in + child_hdr->length) > in_end) {
			/* invalid child data length, stop parsing */
			out = put_header_status(out, out_end, child_hdr->id, NODE_STATUS_ERR_LENGTH);
			break;
		}

		if (child_hdr->id >= n->max_child) {
			/* unknown child id, skip */
			out = put_header_status(out, out_end, child_hdr->id, NODE_STATUS_ERR_ID);
			if (!out) {
				/* no room for response, stop parsing */
				break;
			}

			in += child_hdr->length;
			continue;
		}

		child = n->child[child_hdr->id];

		switch (child->type) {
		case NODE_CONTAINER:
			out = node_iterate(child, child_hdr->id, operation, in, in + child_hdr->length, out, out_end);

			break;

		case NODE_LIST:
			out = list_iterate(child, child_hdr->id, operation, in, in + child_hdr->length, out, out_end);

			break;

		case NODE_LEAF:
			if (operation == NODE_GET)
				out = leaf_get(child, child_hdr->id, out, out_end, base, &status);
			else if (operation == NODE_SET)
				out = leaf_set(child, child_hdr->id, in, in + child_hdr->length, out, out_end, base);

			break;

		case NODE_LEAF_LIST:
			if (operation == NODE_GET)
				out = leaf_list_get(child, in, in + child_hdr->length, out, out_end);
			else if (operation == NODE_SET)
				out = leaf_list_set(child, in, in + child_hdr->length, out, out_end);

			break;

		default:
			/* unknown child type, skip */
			out = put_header_status(out, out_end, child_hdr->id, NODE_STATUS_ERR_TYPE);
			if (!out) {
				/* no room for response, stop parsing */
				goto done;
			}

			break;
		}

		in += child_hdr->length;
	}

done:
	return out;
}

uint8_t *module_iterate(struct module *m, enum node_operation operation, uint8_t *in, uint8_t *in_end, uint8_t *out, uint8_t *out_end)
{
	return child_iterate(&m->n, operation, in, in_end, out, out_end, 0);
}
